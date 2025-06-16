import numpy as np
import matplotlib.pyplot as plt
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import DBSCAN
from sklearn.metrics import (
    adjusted_rand_score, normalized_mutual_info_score,
    homogeneity_score, completeness_score, v_measure_score,
    silhouette_score, confusion_matrix
)
from sklearn.neighbors import NearestNeighbors
from sklearn.datasets import fetch_openml
import umap.umap_ as umap
import time
import warnings

warnings.filterwarnings('ignore')


class MNISTDBSCANClustering:

    def __init__(self, n_components=10, eps=None, min_samples=None,
                 use_emnist=False, subset_size=None):

        self.n_components = n_components
        self.eps = eps
        self.min_samples = min_samples if min_samples else 2 * n_components
        self.use_emnist = use_emnist
        self.subset_size = subset_size

        # Initialize components
        self.scaler = StandardScaler()
        self.reducer = umap.UMAP(
            n_neighbors=30,
            min_dist=0.0,
            n_components=n_components,
            metric='euclidean',
            random_state=42,
            verbose=True
        )
        self.dbscan = None
        self.labels_ = None
        self.X_reduced_ = None

    def load_data(self):
        print(f"Loading {'EMNIST' if self.use_emnist else 'MNIST'} dataset...")

        if self.use_emnist:
            data = fetch_openml('mnist_784', version=1, as_frame=False, parser='auto')
        else:
            data = fetch_openml('mnist_784', version=1, as_frame=False, parser='auto')

        X, y = data['data'], data['target'].astype(int)

        if self.subset_size:
            indices = np.random.RandomState(42).choice(len(X), self.subset_size, replace=False)
            X, y = X[indices], y[indices]

        print(f"Loaded {len(X)} samples")
        return X, y

    def preprocess_data(self, X):
        print("\nPreprocessing data...")

        X_normalized = X.astype('float32') / 255.0

        print("Standardizing features...")
        X_scaled = self.scaler.fit_transform(X_normalized)

        print(f"Applying UMAP reduction to {self.n_components} dimensions...")
        start_time = time.time()
        X_reduced = self.reducer.fit_transform(X_scaled)
        print(f"UMAP completed in {time.time() - start_time:.2f} seconds")

        return X_reduced

    def find_optimal_epsilon(self, X, k=None):
        if k is None:
            k = self.min_samples

        print(f"\nFinding optimal epsilon using {k}-NN distances...")
        neighbors = NearestNeighbors(n_neighbors=k)
        neighbors_fit = neighbors.fit(X)
        distances, indices = neighbors_fit.kneighbors(X)

        distances = np.sort(distances[:, k - 1], axis=0)

        knee_idx = int(0.95 * len(distances))
        optimal_eps = distances[knee_idx]

        plt.figure(figsize=(10, 6))
        plt.plot(distances)
        plt.axhline(y=optimal_eps, color='r', linestyle='--',
                    label=f'Suggested ε = {optimal_eps:.3f}')
        plt.xlabel('Points sorted by distance')
        plt.ylabel(f'{k}-NN Distance')
        plt.title('K-distance Graph for Epsilon Selection')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.show()

        return optimal_eps

    def optimize_parameters(self, X, y, target_clusters=20, max_noise=0.15):
        print(f"\nOptimizing parameters for ~{target_clusters} clusters with <{max_noise * 100}% noise...")

        eps_range = np.arange(0.1, 1.0, 0.05)
        min_samples_range = range(max(5, self.n_components),
                                  min(50, self.n_components * 5), 5)

        best_params = None
        best_score = -1
        results = []

        for eps in eps_range:
            for min_samples in min_samples_range:
                dbscan_temp = DBSCAN(eps=eps, min_samples=min_samples, n_jobs=-1)
                labels = dbscan_temp.fit_predict(X)

                n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
                noise_pct = list(labels).count(-1) / len(labels)

                if n_clusters > 0:
                    ari = adjusted_rand_score(y, labels)

                    results.append({
                        'eps': eps, 'min_samples': min_samples,
                        'n_clusters': n_clusters, 'noise_pct': noise_pct,
                        'ari': ari
                    })

                    # Check if within target range
                    if (target_clusters * 0.5 <= n_clusters <= target_clusters * 1.5
                            and noise_pct <= max_noise and ari > best_score):
                        best_score = ari
                        best_params = {'eps': eps, 'min_samples': min_samples}
                        print(f"  New best: eps={eps:.2f}, min_samples={min_samples}, "
                              f"clusters={n_clusters}, noise={noise_pct:.1%}, ARI={ari:.3f}")

        if best_params:
            print(f"\nBest parameters found: eps={best_params['eps']:.3f}, "
                  f"min_samples={best_params['min_samples']}")
        else:
            print("\nNo parameters found within constraints. Using defaults.")

        return best_params, results

    def cluster(self, X, y, optimize=True):
        self.X_reduced_ = self.preprocess_data(X)

        if self.eps is None:
            if optimize:
                params, _ = self.optimize_parameters(self.X_reduced_, y)
                if params:
                    self.eps = params['eps']
                    self.min_samples = params['min_samples']
                else:
                    self.eps = self.find_optimal_epsilon(self.X_reduced_)
            else:
                self.eps = self.find_optimal_epsilon(self.X_reduced_)

        print(f"\nClustering with eps={self.eps:.3f}, min_samples={self.min_samples}...")
        start_time = time.time()

        self.dbscan = DBSCAN(
            eps=self.eps,
            min_samples=self.min_samples,
            algorithm='ball_tree',
            n_jobs=-1
        )
        self.labels_ = self.dbscan.fit_predict(self.X_reduced_)

        print(f"Clustering completed in {time.time() - start_time:.2f} seconds")

        return self.labels_

    def calculate_cluster_purity(self, y_true, labels):
        cluster_purities = {}

        mask = labels != -1
        y_true_clean = y_true[mask]
        labels_clean = labels[mask]

        for cluster_id in np.unique(labels_clean):
            cluster_mask = labels_clean == cluster_id
            cluster_true_labels = y_true_clean[cluster_mask]

            unique_labels, counts = np.unique(cluster_true_labels, return_counts=True)
            dominant_label = unique_labels[np.argmax(counts)]
            purity = np.max(counts) / len(cluster_true_labels)

            cluster_purities[cluster_id] = {
                'purity': purity,
                'dominant_label': dominant_label,
                'size': len(cluster_true_labels),
                'label_distribution': dict(zip(unique_labels, counts))
            }

        return cluster_purities

    def evaluate(self, y_true):
        if self.labels_ is None:
            raise ValueError("No clustering results. Run cluster() first.")

        labels = self.labels_

        n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
        n_noise = list(labels).count(-1)
        noise_percentage = (n_noise / len(labels)) * 100

        ari = adjusted_rand_score(y_true, labels)
        nmi = normalized_mutual_info_score(y_true, labels)
        homogeneity = homogeneity_score(y_true, labels)
        completeness = completeness_score(y_true, labels)
        v_measure = v_measure_score(y_true, labels)

        if n_clusters > 1 and n_noise < len(labels) - 1:
            mask = labels != -1
            silhouette = silhouette_score(self.X_reduced_[mask], labels[mask])
        else:
            silhouette = None

        mask = labels != -1
        y_true_clean = y_true[mask]
        labels_clean = labels[mask]

        total_correct = 0
        for cluster_id in np.unique(labels_clean):
            cluster_mask = labels_clean == cluster_id
            cluster_true_labels = y_true_clean[cluster_mask]
            most_common = np.bincount(cluster_true_labels).argmax()
            total_correct += np.sum(cluster_true_labels == most_common)

        overall_purity = total_correct / len(y_true_clean) if len(y_true_clean) > 0 else 0

        # Get cluster purities
        cluster_purities = self.calculate_cluster_purity(y_true, labels)

        return {
            'n_clusters': n_clusters,
            'n_noise': n_noise,
            'noise_percentage': noise_percentage,
            'adjusted_rand_index': ari,
            'normalized_mutual_info': nmi,
            'homogeneity': homogeneity,
            'completeness': completeness,
            'v_measure': v_measure,
            'silhouette_score': silhouette,
            'overall_purity': overall_purity,
            'cluster_purities': cluster_purities
        }

    def print_results(self, metrics):
        print("\n" + "=" * 60)
        print("DBSCAN CLUSTERING RESULTS")
        print("=" * 60)

        print(f"\n📊 CLUSTER STATISTICS:")
        print(f"   Number of clusters: {metrics['n_clusters']}")
        print(f"   Noise points: {metrics['n_noise']} ({metrics['noise_percentage']:.2f}%)")
        print(f"   Clustered points: {100 - metrics['noise_percentage']:.2f}%")

        print(f"\n📈 PERFORMANCE METRICS:")
        print(f"   Adjusted Rand Index (ARI): {metrics['adjusted_rand_index']:.4f}")
        print(f"   Normalized Mutual Info (NMI): {metrics['normalized_mutual_info']:.4f}")
        print(f"   Homogeneity: {metrics['homogeneity']:.4f}")
        print(f"   Completeness: {metrics['completeness']:.4f}")
        print(f"   V-measure: {metrics['v_measure']:.4f}")
        if metrics['silhouette_score']:
            print(f"   Silhouette Score: {metrics['silhouette_score']:.4f}")

        print(f"\nCLASSIFICATION ACCURACY:")
        print(f"   Overall cluster purity: {metrics['overall_purity']:.2%}")
        print(f"   Misclassification rate: {(1 - metrics['overall_purity']):.2%}")

        print(f"\n📋 CLUSTER DETAILS:")
        purities = metrics['cluster_purities']
        for cluster_id in sorted(purities.keys()):
            info = purities[cluster_id]
            print(f"   Cluster {cluster_id}: "
                  f"size={info['size']}, "
                  f"purity={info['purity']:.2%}, "
                  f"dominant_digit={info['dominant_label']}")

        print("\n" + "=" * 60)

    def visualize_clusters(self, y_true, n_samples=5000):
        """Visualize clustering results"""
        if self.X_reduced_ is None or self.labels_ is None:
            raise ValueError("No clustering results to visualize.")

        # Use subset for visualization if data is large
        if len(self.X_reduced_) > n_samples:
            indices = np.random.choice(len(self.X_reduced_), n_samples, replace=False)
            X_vis = self.X_reduced_[indices]
            labels_vis = self.labels_[indices]
            y_vis = y_true[indices]
        else:
            X_vis = self.X_reduced_
            labels_vis = self.labels_
            y_vis = y_true

        # If dimensionality > 2, use first 2 components
        if X_vis.shape[1] > 2:
            X_2d = X_vis[:, :2]
        else:
            X_2d = X_vis

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

        # Plot 1: Cluster assignments
        scatter1 = ax1.scatter(X_2d[:, 0], X_2d[:, 1], c=labels_vis,
                               cmap='tab20', s=10, alpha=0.6)
        ax1.set_title('DBSCAN Cluster Assignments')
        ax1.set_xlabel('UMAP Component 1')
        ax1.set_ylabel('UMAP Component 2')

        # Plot 2: True labels
        scatter2 = ax2.scatter(X_2d[:, 0], X_2d[:, 1], c=y_vis,
                               cmap='tab10', s=10, alpha=0.6)
        ax2.set_title('True Digit Labels')
        ax2.set_xlabel('UMAP Component 1')
        ax2.set_ylabel('UMAP Component 2')

        plt.colorbar(scatter1, ax=ax1, label='Cluster ID')
        plt.colorbar(scatter2, ax=ax2, label='Digit')
        plt.tight_layout()
        plt.show()


def main():
    """Main execution function"""
    print("DBSCAN Clustering for MNIST/EMNIST Dataset")
    print("=" * 60)

    # Configuration
    USE_FULL_DATASET = False  # Set to True for full dataset
    SUBSET_SIZE = 10000 if not USE_FULL_DATASET else None
    OPTIMIZE_PARAMS = True  # Set to False for faster execution

    # Initialize clusterer
    clusterer = MNISTDBSCANClustering(
        n_components=10,  # UMAP dimensions
        eps=None,  # Auto-determine
        min_samples=None,  # Auto-determine
        use_emnist=False,  # Use standard MNIST
        subset_size=SUBSET_SIZE
    )

    X, y = clusterer.load_data()

    labels = clusterer.cluster(X, y, optimize=OPTIMIZE_PARAMS)

    metrics = clusterer.evaluate(y)

    clusterer.print_results(metrics)

    print("\nGenerating visualizations...")
    clusterer.visualize_clusters(y)

    return clusterer, metrics


if __name__ == "__main__":
    clusterer, metrics = main()

    # Additional analysis can be performed here
    print("\n✅ Clustering complete!")
    print(f"Parameters used: eps={clusterer.eps:.3f}, min_samples={clusterer.min_samples}")