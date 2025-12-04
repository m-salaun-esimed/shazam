# 🎵 Shazam Local - Système de Reconnaissance Audio

![Version](https://img.shields.io/badge/version-2.0-green)
![C++](https://img.shields.io/badge/C++-23-blue)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

## Table des matières
1. [Vue d'ensemble](#vue-densemble)
2. [Fonctionnalités](#fonctionnalités)
3. [Architecture du projet](#architecture-du-projet)
4. [Concepts scientifiques](#concepts-scientifiques)
5. [Pipeline algorithmique](#pipeline-algorithmique)
6. [Structures de données](#structures-de-données)
7. [Complexité algorithmique](#complexité-algorithmique)
8. [Utilisation](#utilisation)
9. [Optimisations](#optimisations)

---

## Vue d'ensemble

Ce projet implémente un **système de reconnaissance audio local** inspiré de Shazam avec une **interface graphique moderne**. Il permet d'identifier une chanson à partir d'un court extrait audio (5 secondes suffisent) en la comparant à une base de données de chansons indexées.

**Principe** : Transformer un signal audio en "empreintes digitales" (fingerprints) uniques, puis rechercher ces empreintes dans une base de données.

**Nouveautés v2.0** :
- ✨ Interface graphique moderne avec SFML (design sombre inspiré de Spotify)
- ⚡ Optimisation de recherche : traite uniquement les 5 premières secondes pour une identification rapide
- 🎯 Filtrage intelligent : affiche uniquement les matches avec confiance >= 90%
- 🎨 Effets visuels : hover sur boutons, barre de progression animée, code couleur pour résultats

---

## Fonctionnalités

### 🎵 Reconnaissance Audio
- **Identification rapide** : 5 secondes d'audio suffisent pour identifier une chanson
- **Haute précision** : Affiche uniquement les matches avec confiance >= 90%
- **Robuste au bruit** : Tolère les variations de volume et les petites distorsions

### 💾 Gestion de Base de Données
- **Indexation automatique** : Scanne tous les fichiers WAV du dossier `/data`
- **Format simple** : Base de données texte lisible et modifiable
- **Métadonnées intelligentes** : Extraction automatique artiste/titre depuis le nom de fichier

### 🎨 Interface Graphique Moderne
- **Design sombre** : Interface élégante inspirée de Spotify
- **Feedback visuel** : Barre de progression, effets hover, coloration des résultats
- **Navigation intuitive** : Menu principal avec accès rapide à toutes les fonctions

---

## Architecture du projet

```
shazam/
├── include/
│   ├── audio.h         # Chargement et traitement des fichiers WAV
│   ├── fft.h           # Transformée de Fourier rapide (FFT)
│   ├── fingerprint.h   # Génération des empreintes audio + optimisations
│   ├── database.h      # Base de données locale avec cache
│   └── gui.h           # Interface graphique SFML
├── src/
│   ├── audio.cpp       # Traitement du signal (mono, frames, fenêtrage)
│   ├── fft.cpp         # Algorithme Cooley-Tukey + extraction peaks
│   ├── fingerprint.cpp # Hash generation + query optimization
│   ├── database.cpp    # Indexation, recherche, persistence
│   ├── gui.cpp         # Interface utilisateur complète
│   └── main.cpp        # Point d'entrée
├── data/               # Dossier contenant les fichiers WAV à indexer
├── shazam.db           # Base de données (format texte)
└── CMakeLists.txt      # Configuration de build
```

---

## Concepts scientifiques

### 1. Signal Audio Numérique

Un fichier audio WAV contient :
- **Samples** : Valeurs numériques représentant l'amplitude du son à des intervalles réguliers
- **Sample Rate** : Nombre de samples par seconde (ex: 44100 Hz = CD quality)
- **Canaux** : Mono (1) ou Stéréo (2)

**Dans le code** : Conversion automatique en mono en moyennant les canaux (src/audio.cpp:42-67)

```
Signal stéréo → Signal mono
[L, R] → (L + R) / 2
```

### 2. Fenêtrage (Windowing)

Le signal audio est découpé en **frames** (morceaux) pour analyse.

**Paramètres utilisés** :
- **Frame size** : 4096 samples (~93 ms à 44100 Hz)
- **Hop size** : 2048 samples (overlap de 50%)

**Fenêtre de Hann** : Fonction mathématique appliquée pour réduire les artefacts spectraux
```
w[n] = 0.5 * (1 - cos(2π * n / (N-1)))
```

**Pourquoi ?** : Évite les discontinuités aux bords des frames qui créent des fréquences parasites.

**Implémentation** : src/audio.cpp:69-99

### 3. Transformée de Fourier Rapide (FFT)

La FFT transforme un signal temporel en signal fréquentiel.

**Principe** : Décompose le son en ses fréquences constituantes
```
Signal temporel (amplitude vs temps) → Signal fréquentiel (magnitude vs fréquence)
```

**Algorithme de Cooley-Tukey** (src/fft.cpp:8-51) :
- Divise récursivement le problème en deux
- Complexité : O(N log N) au lieu de O(N²)
- Nécessite que N soit une puissance de 2

**Formule mathématique** :
```
X[k] = Σ(n=0 to N-1) x[n] * e^(-2πikn/N)
```

Où :
- `X[k]` = composante fréquentielle k
- `x[n]` = sample temporel n
- `i` = nombre imaginaire (racine de -1)

**Résultat** : Nombres complexes (parties réelle et imaginaire) représentant chaque fréquence.

### 4. Spectre de magnitude

La magnitude représente l'intensité de chaque fréquence :
```
magnitude[k] = sqrt(real[k]² + imag[k]²)
```

**Implémentation** : src/fft.cpp:53-64

### 5. Extraction des pics (Peaks)

Un **pic** est une fréquence localement maximale dans le spectre.

**Critères de sélection** (src/fft.cpp:66-105) :
1. Magnitude > seuil (threshold = 1% du max)
2. Plus grand que ses voisins immédiats
3. Garde seulement les N plus forts (top 5 par frame)

**Pourquoi ?** : Réduit le bruit et conserve uniquement les caractéristiques distinctives.

### 6. Fingerprinting (Empreintes audio)

Un **fingerprint** est un hash unique représentant une combinaison de pics.

**Algorithme** (src/fingerprint.cpp:8-66) :

Pour chaque pic "anchor" (ancre) :
1. Trouve les pics dans les frames suivantes (fenêtre de 5 frames, max 3 pics par frame)
2. Calcule un hash avec :
   - Fréquence du pic anchor
   - Fréquence du pic cible
   - Delta temporel entre eux

**Formule du hash** :
```cpp
hash = (f1 & 0xFFFF) | ((f2 & 0xFFFF) << 16) | ((dt & 0xFFFFFFFF) << 32)
```

Où :
- `f1` = fréquence anchor (16 bits)
- `f2` = fréquence cible (16 bits)
- `dt` = delta temporel (32 bits)

**Structure du fingerprint** :
```cpp
struct Fingerprint {
    uint64_t hash;      // Hash unique
    int timeOffset;     // Position dans la chanson
}
```

**Avantages** :
- Robuste au bruit (seuls les pics les plus forts comptent)
- Résistant aux variations de volume
- Tolère les petites distorsions

---

## Pipeline algorithmique

### Phase 1 : Indexation (ajouter une chanson à la base)

```
Fichier WAV
    ↓
1. Chargement et conversion mono
    ↓
2. Découpage en frames (4096 samples, hop 2048)
    ↓
3. Application de la fenêtre de Hann
    ↓
4. FFT sur chaque frame
    ↓
5. Calcul des magnitudes
    ↓
6. Extraction des pics (top 5 par frame)
    ↓
7. Génération des fingerprints (combinaisons de pics)
    ↓
8. Stockage en base de données
    {songId, hash, timeOffset}
```

**Code** : src/main.cpp:61-124

### Phase 2 : Recherche (identifier un extrait)

```
Extrait audio (query)
    ↓
1-7. Même processus que l'indexation
    ↓
8. Pour chaque fingerprint de la query :
    a. Chercher le hash dans l'index
    b. Pour chaque match trouvé :
       - Calculer offsetDelta = dbTimeOffset - queryTimeOffset
       - Incrémenter compteur[songId, offsetDelta]
    ↓
9. Trier les résultats par score
    ↓
10. Calculer le % de confiance
    confidence = (nb_matches / nb_query_fingerprints) * 100
```

**Code** : src/main.cpp:127-169 et src/database.cpp:129-162

---

## Structures de données

### 1. Base de données en mémoire

**Index des fingerprints** :
```cpp
std::unordered_map<uint32_t, std::vector<StoredFingerprint>> fingerprintIndex;
```

- **Clé** : hash du fingerprint (uint32_t)
- **Valeur** : liste des occurrences {songId, timeOffset}
- **Complexité de recherche** : O(1) en moyenne (hash map)

**Stockage des chansons** :
```cpp
std::map<int, Song> songs;
```

- **Clé** : ID de la chanson
- **Valeur** : métadonnées {id, title, artist, duration}
- **Complexité de recherche** : O(log N) (arbre binaire)

### 2. Persistence (format texte)

Format du fichier `shazam.db` :

```
[SONGS]
# id|title|artist|duration
1|Past Self|Sleep Token|234.5

[FINGERPRINTS]
# hash|songId|timeOffset
1234567890|1|42
9876543210|1|43
```

**Avantages** :
- Lisible et modifiable
- Pas de dépendance externe (SQLite)
- Simple à parser

**Inconvénients** :
- Plus lent que SQL pour les grandes bases
- Chargement complet en mémoire

---

## Complexité algorithmique

### Indexation d'une chanson

Soit :
- `N` = nombre de samples
- `F` = nombre de frames = N / hop_size
- `P` = nombre de pics par frame = 5
- `W` = fenêtre de recherche = 5 frames
- `T` = pics ciblés par anchor = 3

**Étapes** :
1. Chargement WAV : O(N)
2. Découpage frames : O(N)
3. FFT sur F frames : O(F × 4096 × log(4096))
4. Extraction pics : O(F × 4096)
5. Génération fingerprints : O(F × P × W × T) = O(F)
6. Sauvegarde en base : O(F × P × W × T)

**Complexité totale** : **O(F × log(4096))** ≈ O(N × log(N))

### Recherche

Soit :
- `Q` = nombre de fingerprints de la query (~1000-2000)
- `D` = nombre de chansons en base
- `M` = nombre moyen de matches par hash

**Étapes** :
1. Extraction fingerprints query : O(N × log(N))
2. Recherche dans l'index : O(Q × M × log(D))
3. Tri des résultats : O(D × log(D))

**Complexité totale** : **O(N × log(N) + Q × M × log(D))**

En pratique, M est petit (1-10) donc la recherche est rapide.

---

## Utilisation

### Installation et Compilation

#### Prérequis
- **Compilateur C++23** (GCC 12+, Clang 15+, MSVC 2022+)
- **CMake 3.20+**
- **SFML 2.6+** (pour l'interface graphique)

#### Windows (Visual Studio / MinGW)
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Linux
```bash
mkdir build && cd build
cmake ..
make -j4
```

### Préparation des fichiers

Place tes fichiers WAV dans le dossier `data/` :

**Format du nom** : `Artiste_Titre.wav`

Exemples :
- `Sleep_Token_Past_Self.wav`
- `Radiohead_Creep.wav`
- `Pink_Floyd_Comfortably_Numb.wav`

**Format audio supporté** : WAV 16-bit, mono ou stéréo, 44100 Hz (recommandé)

### Lancement

```bash
./shazam         # Linux/Mac
shazam.exe       # Windows
```

### Interface Graphique

L'application s'ouvre avec un **menu principal** offrant 5 options :

1. **📁 Indexer tous les fichiers** : Scanne et indexe tous les WAV du dossier `/data`
2. **🔍 Identifier un audio** : Ouvre un dialogue pour sélectionner un fichier à identifier
3. **📋 Voir les chansons en base** : Affiche la liste de toutes les chansons indexées
4. **🗑️ Vider la base** : Supprime toutes les données indexées
5. **❌ Quitter** : Ferme l'application

### Workflow typique

#### Première utilisation
1. Lance l'application
2. Clique sur **"Indexer tous les fichiers"**
3. Attends la fin de l'indexation (barre de progression)
4. Les chansons sont maintenant en base de données !

#### Identifier une chanson
1. Clique sur **"Identifier un audio"**
2. Sélectionne un fichier WAV (même un court extrait de 5-10 secondes)
3. L'analyse se lance automatiquement
4. Les résultats s'affichent avec :
   - **Titre et artiste** en vert
   - **Score et confiance** en gris
   - **"MATCH CONFIRME!"** si confiance >= 90%

**Interprétation des résultats** :
- **≥ 90%** : Match confirmé ✅ (seuls ces résultats sont affichés)
- **< 90%** : "Aucun son trouvé" (la chanson n'est pas en base ou qualité insuffisante)

---

## Optimisations

### ⚡ Optimisations Implémentées (v2.0)

1. **Recherche rapide** : Traite uniquement les 5 premières secondes pour les queries (36x plus rapide)
2. **Cache de fingerprints** : Stocke le nombre de fingerprints par chanson pour calcul rapide de confiance
3. **Filtrage intelligent** : N'affiche que les matches >= 90% de confiance
4. **Interface réactive** : Mise à jour de l'UI toutes les 100 frames pendant l'indexation

### 🚀 Optimisations Futures Possibles

#### Performances
1. **FFT optimisée** : Utiliser FFTW (bibliothèque C hautement optimisée)
2. **Parallélisation** : Traiter plusieurs frames en parallèle avec OpenMP
3. **Base SQL** : Migrer vers SQLite avec index B-tree pour bases > 1000 chansons
4. **Batch save** : Sauvegarder en base seulement à la fin de l'indexation
5. **Memory mapping** : Utiliser mmap pour chargement rapide de la base

#### Robustesse
1. **Pitch shifting** : Gérer les variations de tonalité (transposition)
2. **Time stretching** : Résister aux variations de tempo
3. **Noise reduction** : Filtrage passe-bande avant FFT
4. **Adaptive threshold** : Ajuster dynamiquement le seuil de détection des pics
5. **Multi-format** : Support MP3, FLAC, OGG via libsndfile

---

## Mathématiques derrière Shazam

### Théorème de Fourier

> Toute fonction périodique peut être décomposée en une somme de sinusoïdes

Un son complexe = addition de fréquences pures (sinus/cosinus)

### Théorème de Nyquist-Shannon

> Pour capturer une fréquence f, le sample rate doit être ≥ 2f

44100 Hz → peut capturer jusqu'à 22050 Hz (limite de l'audition humaine ≈ 20000 Hz)

### Résolution fréquentielle

Avec une frame de 4096 samples à 44100 Hz :

```
Résolution = sample_rate / frame_size
           = 44100 / 4096
           ≈ 10.8 Hz
```

Chaque bin FFT représente ~10.8 Hz.

### Résolution temporelle

Avec hop_size = 2048 :

```
Résolution temporelle = hop_size / sample_rate
                      = 2048 / 44100
                      ≈ 46 ms
```

Une nouvelle frame tous les 46 ms.

**Trade-off** : Frame grande = bonne résolution fréquentielle mais mauvaise résolution temporelle, et vice-versa.

---

## Limites Connues

1. **Scalabilité** : Base en mémoire limitée à ~500-1000 chansons (selon RAM)
2. **Format audio** : Supporte uniquement WAV 16-bit
3. **Robustesse** : Sensible aux variations importantes de tempo/pitch
4. **Stockage** : Format texte inefficace pour très grandes bases (> 10 000 chansons)
5. **Plateforme** : File dialog Windows uniquement (Linux/Mac nécessitent adaptation)

**Objectif** : Implémentation éducative démontrant les principes fondamentaux de la reconnaissance audio.

---


## Auteur
**SALAUN Matthieu**

Projet développé dans le cadre du cours de C++ moderne - ESIMED
Décembre 2024

**Technologies** :
- C++23 (std::expected, ranges, concepts)
- CMake 3.20+
- SFML 2.6 (Graphics, Window)
- STL (unordered_map, vector, algorithms)
- FFT custom (Cooley-Tukey)