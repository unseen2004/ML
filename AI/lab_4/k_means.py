import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import fetch_openml


def initialize_centroids_pp(X, k, rng=None):
    """
    Inicjalizacja centroidów za pomocą algorytmu k-means.

    Parametry:
    - X: ndarray o kształcie (n_samples, n_features) - dane wejściowe
    - k: int - liczba klastrów
    - rng: np.random.RandomState lub None - generator liczb losowych

    Zwraca:
    - centroids: ndarray o kształcie (k, n_features) - zainicjalizowane centroidy
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
    Przypisz każdą próbkę w X do najbliższego centroidu.

    Parametry:
    - X: ndarray o kształcie (n_samples, n_features) - dane wejściowe
    - centroids: ndarray o kształcie (k, n_features) - centroidy

    Zwraca:
    - labels: ndarray o kształcie (n_samples,) - indeksy klastrów dla każdej próbki
    """
    # Oblicz kwadraty odległości między próbkami a centroidami: (n_samples, k)
    dists = np.sum((X[:, None, :] - centroids[None, :, :])**2, axis=2)
    # Zwróć indeks najbliższego centroidu dla każdej próbki
    return np.argmin(dists, axis=1)


def compute_centroids(X, labels, k):
    """
    Oblicz centroidy jako średnią próbek w każdym klastrze.

    Parametry:
    - X: ndarray o kształcie (n_samples, n_features) - dane wejściowe
    - labels: ndarray o kształcie (n_samples,) - indeksy klastrów
    - k: int - liczba klastrów

    Zwraca:
    - centroids: ndarray o kształcie (k, n_features) - nowe centroidy
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
    Oblicz inercję jako sumę kwadratów odległości próbek do ich centroidów.

    Parametry:
    - X: ndarray o kształcie (n_samples, n_features) - dane wejściowe
    - labels: ndarray o kształcie (n_samples,) - indeksy klastrów
    - centroids: ndarray o kształcie (k, n_features) - centroidy

    Zwraca:
    - out: float - wartość inercji
    """
    out = 0.0
    for j in range(centroids.shape[0]):
        members = X[labels == j]
        out += np.sum((members - centroids[j])**2)
    return out


def kmeans(X, k, max_iters=100, tol=1e-4, n_init=5, random_state=None):
    """
    Wykonaj klasteryzację k-means z inicjalizacją k-means++ i wieloma restartami.

    Parametry:
    - X: ndarray o kształcie (n_samples, n_features) - dane wejściowe
    - k: int - liczba klastrów
    - max_iters: int - maksymalna liczba iteracji
    - tol: float - tolerancja przesunięcia centroidów
    - n_init: int - liczba restartów
    - random_state: int lub None - ziarno generatora liczb losowych

    Zwraca:
    - best_labels: ndarray o kształcie (n_samples,)
    - best_centroids: ndarray o kształcie (k, n_features)
    - best_inertia: float
    """
    best_inertia = np.inf
    best_centroids = None
    best_labels = None
    rng = np.random.RandomState(random_state)

    for init_no in range(n_init):
        # Inicjalizuj centroidy
        centroids = initialize_centroids_pp(X, k, rng)
        for it in range(max_iters):
            # Przypisz próbki do klastrów
            labels = assign_clusters(X, centroids)
            # Oblicz nowe centroidy
            new_centroids = compute_centroids(X, labels, k)
            # Oblicz przesunięcie centroidów
            shift = np.linalg.norm(new_centroids - centroids)
            centroids = new_centroids
            # Jeśli przesunięcie jest mniejsze niż tolerancja, zakończ iterację
            if shift < tol:
                break
        # Oblicz inercję dla bieżącego rozwiązania
        inertia = compute_inertia(X, labels, centroids)
        # Zaktualizuj najlepsze rozwiązanie, jeśli inercja jest mniejsza
        if inertia < best_inertia:
            best_inertia = inertia
            best_centroids = centroids.copy()
            best_labels = labels.copy()
    return best_labels, best_centroids, best_inertia


def plot_assignment_matrix(labels, truths, k, save_path=None):
    """
    Narysuj macierz (k x 10): rozkład klas cyfr w każdym klastrze.

    Parametry:
    - labels: ndarray o kształcie (n_samples,) - przypisania klastrów
    - truths: ndarray o kształcie (n_samples,) - prawdziwe etykiety cyfr
    - k: int - liczba klastrów
    - save_path: str lub None - ścieżka do zapisania wykresu (jeśli None, wyświetl wykres)
    """
    matrix = np.zeros((k, 10), dtype=float)
    for cluster in range(k):
        # Wybierz próbki należące do klastra
        cluster_idx = labels == cluster
        total = np.sum(cluster_idx)
        if total > 0:
            for digit in range(10):
                # Oblicz procent próbek w klastrze, które należą do danej cyfry
                matrix[cluster, digit] = np.sum(truths[cluster_idx] == digit) / total * 100
    plt.figure(figsize=(8, 6))
    plt.imshow(matrix, aspect='auto', interpolation='nearest', origin='lower')
    plt.colorbar(label='Procent (%)')
    plt.xlabel('Prawdziwa cyfra')
    plt.ylabel('Indeks klastra')
    plt.title(f'Rozkład prawdziwych cyfr w klastrach (k={k})')
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
    Narysuj obrazy centroidów w siatce.

    Parametry:
    - centroids: ndarray o kształcie (k, n_features) - centroidy
    - img_shape: tuple - kształt obrazu (np. (28, 28) dla MNIST)
    - save_path: str lub None - ścieżka do zapisania wykresu (jeśli None, wyświetl wykres)
    """
    k = centroids.shape[0]
    cols = int(np.ceil(np.sqrt(k)))
    rows = int(np.ceil(k / cols))
    plt.figure(figsize=(cols * 2, rows * 2))
    for i in range(k):
        plt.subplot(rows, cols, i + 1)
        plt.imshow(centroids[i].reshape(img_shape), cmap='gray', interpolation='nearest')
        plt.axis('off')
        plt.title(f'Klastra {i}')
    plt.suptitle(f'Obrazy centroidów (k={k})', y=1.02)
    plt.tight_layout()
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
    else:
        plt.show()
    plt.close()


def main():
    # Wczytaj dane EMNIST MNIST (70000 cyfr)
    print('Wczytywanie danych...')
    mnist = fetch_openml('mnist_784', version=1)
    X = mnist.data.values.astype(np.float64) / 255.0
    y = mnist.target.astype(int).values

    ks = [10, 15, 20, 30]
    results = {}
    for k in ks:
        print(f'Klasteryzacja dla k={k}...')
        labels, centroids, inertia = kmeans(X, k, n_init=5, random_state=42)
        print(f' - Najlepsza inercja: {inertia:.2f}')
        # Zapisz wykresy
        plot_assignment_matrix(labels, y, k, save_path=f'assignment_k{k}.png')
        plot_centroids(centroids, (28, 28), save_path=f'centroids_k{k}.png')
        results[k] = {'labels': labels, 'centroids': centroids, 'inertia': inertia}

    # Przykład: podsumowanie wyników
    print('\nPodsumowanie inercji:')
    for k, res in results.items():
        print(f'k={k}: inercja={res["inertia"]:.2f}')


if __name__ == '__main__':
    main()