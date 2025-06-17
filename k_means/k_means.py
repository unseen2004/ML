import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import fetch_openml


def initialize_centroids_pp(X, k, rng=None):
    """
    Initialize centroids using the k-means++ algorithm.

    Parameters:
    - X: ndarray of shape (n_samples, n_features) - input data
    - k: int - number of clusters
    - rng: np.random.RandomState or None - random number generator

    Returns:
    - centroids: ndarray of shape (k, n_features) - initialized centroids
    """
    if rng is None:
        rng = np.random.RandomState()
    n_samples, _ = X.shape
    centroids = np.empty((k, X.shape[1]), dtype=X.dtype)
    idx = rng.randint(n_samples)
    centroids[0] = X[idx]
    dist_sq = np.full(n_samples, np.inf)
    for i in range(1, k):
        diff = X - centroids[i-1]
        new_dist_sq = np.sum(diff**2, axis=1)
        dist_sq = np.minimum(dist_sq, new_dist_sq)
        probs = dist_sq / np.sum(dist_sq)
        cumulative = np.cumsum(probs)
        r = rng.rand()
        idx = np.searchsorted(cumulative, r)
        centroids[i] = X[idx]
    return centroids


def assign_clusters(X, centroids):
    """
    Assign each sample in X to the nearest centroid.

    Parameters:
    - X: ndarray of shape (n_samples, n_features) - input data
    - centroids: ndarray of shape (k, n_features) - centroids

    Returns:
    - labels: ndarray of shape (n_samples,) - cluster indices for each sample
    """
    # Compute squared distances between samples and centroids: (n_samples, k)
    dists = np.sum((X[:, None, :] - centroids[None, :, :])**2, axis=2)
    # Return the index of the nearest centroid for each sample
    return np.argmin(dists, axis=1)


def compute_centroids(X, labels, k):
    """
    Compute centroids as the mean of samples in each cluster.

    Parameters:
    - X: ndarray of shape (n_samples, n_features) - input data
    - labels: ndarray of shape (n_samples,) - cluster indices
    - k: int - number of clusters

    Returns:
    - centroids: ndarray of shape (k, n_features) - new centroids
    """
    n_features = X.shape[1]
    centroids = np.zeros((k, n_features), dtype=X.dtype)
    for j in range(k):
        members = X[labels == j]
        if len(members) > 0:
            centroids[j] = np.mean(members, axis=0)
        else:
            centroids[j] = X[np.random.randint(0, X.shape[0])]
    return centroids


def compute_inertia(X, labels, centroids):
    """
    Compute inertia as the sum of squared distances of samples to their centroids.

    Parameters:
    - X: ndarray of shape (n_samples, n_features) - input data
    - labels: ndarray of shape (n_samples,) - cluster indices
    - centroids: ndarray of shape (k, n_features) - centroids

    Returns:
    - out: float - inertia value
    """
    out = 0.0
    for j in range(centroids.shape[0]):
        members = X[labels == j]
        out += np.sum((members - centroids[j])**2)
    return out


def kmeans(X, k, max_iters=100, tol=1e-4, n_init=5, random_state=None):
    """
    Perform k-means clustering with k-means++ initialization and multiple restarts.

    Parameters:
    - X: ndarray of shape (n_samples, n_features) - input data
    - k: int - number of clusters
    - max_iters: int - maximum number of iterations
    - tol: float - tolerance for centroid shift
    - n_init: int - number of restarts
    - random_state: int or None - random seed

    Returns:
    - best_labels: ndarray of shape (n_samples,)
    - best_centroids: ndarray of shape (k, n_features)
    - best_inertia: float
    """
    best_inertia = np.inf
    best_centroids = None
    best_labels = None
    rng = np.random.RandomState(random_state)

    for init_no in range(n_init):
        # Initialize centroids
        centroids = initialize_centroids_pp(X, k, rng)
        for it in range(max_iters):
            # Assign samples to clusters
            labels = assign_clusters(X, centroids)
            # Compute new centroids
            new_centroids = compute_centroids(X, labels, k)
            # Compute centroid shift
            shift = np.linalg.norm(new_centroids - centroids)
            centroids = new_centroids
            # Stop iteration if shift is less than tolerance
            if shift < tol:
                break
        # Compute inertia for the current solution
        inertia = compute_inertia(X, labels, centroids)
        # Update the best solution if inertia is lower
        if inertia < best_inertia:
            best_inertia = inertia
            best_centroids = centroids.copy()
            best_labels = labels.copy()
    return best_labels, best_centroids, best_inertia


def plot_assignment_matrix(labels, truths, k, save_path=None):
    """
    Plot a matrix (k x 10): distribution of digit classes in each cluster.

    Parameters:
    - labels: ndarray of shape (n_samples,) - cluster assignments
    - truths: ndarray of shape (n_samples,) - true digit labels
    - k: int - number of clusters
    - save_path: str or None - path to save the plot (if None, display the plot)
    """
    matrix = np.zeros((k, 10), dtype=float)
    for cluster in range(k):
        # Select samples belonging to the cluster
        cluster_idx = labels == cluster
        total = np.sum(cluster_idx)
        if total > 0:
            for digit in range(10):
                # Compute the percentage of samples in the cluster belonging to the digit
                matrix[cluster, digit] = np.sum(truths[cluster_idx] == digit) / total * 100
    plt.figure(figsize=(8, 6))
    plt.imshow(matrix, aspect='auto', interpolation='nearest', origin='lower')
    plt.colorbar(label='Percentage (%)')
    plt.xlabel('True digit')
    plt.ylabel('Cluster index')
    plt.title(f'Distribution of true digits in clusters (k={k})')
    plt.xticks(np.arange(10))
    plt.yticks(np.arange(k))
    plt.tight_layout()
    if save_path:
        plt.savefig(save_path, dpi=150)
    else:
        plt.show()
    plt.close()


def plot_centroids(centroids, img_shape, save_path=None):
    """
    Plot centroid images in a grid.

    Parameters:
    - centroids: ndarray of shape (k, n_features) - centroids
    - img_shape: tuple - image shape (e.g., (28, 28) for MNIST)
    - save_path: str or None - path to save the plot (if None, display the plot)
    """
    k = centroids.shape[0]
    cols = int(np.ceil(np.sqrt(k)))
    rows = int(np.ceil(k / cols))
    plt.figure(figsize=(cols * 2, rows * 2))
    for i in range(k):
        plt.subplot(rows, cols, i + 1)
        plt.imshow(centroids[i].reshape(img_shape), cmap='gray', interpolation='nearest')
        plt.axis('off')
        plt.title(f'Cluster {i}')
    plt.suptitle(f'Centroid images (k={k})', y=1.02)
    plt.tight_layout()
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
    else:
        plt.show()
    plt.close()


def main():
    # Load EMNIST MNIST data (70,000 digits)
    print('Loading data...')
    mnist = fetch_openml('mnist_784', version=1)
    X = mnist.data.values.astype(np.float64) / 255.0
    y = mnist.target.astype(int).values

    ks = [10, 15, 20, 30]
    results = {}
    for k in ks:
        print(f'Clustering for k={k}...')
        labels, centroids, inertia = kmeans(X, k, n_init=5, random_state=42)
        print(f' - Best inertia: {inertia:.2f}')
        # Save plots
        plot_assignment_matrix(labels, y, k, save_path=f'assignment_k{k}.png')
        plot_centroids(centroids, (28, 28), save_path=f'centroids_k{k}.png')
        results[k] = {'labels': labels, 'centroids': centroids, 'inertia': inertia}

    # Example: summary of results
    print('\nSummary of inertia:')
    for k, res in results.items():
        print(f'k={k}: inertia={res["inertia"]:.2f}')


if __name__ == '__main__':
    main()