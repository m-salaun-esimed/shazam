# Shazam Local - Système de Reconnaissance Audio

![Version](https://img.shields.io/badge/version-2.0-green)
![C++](https://img.shields.io/badge/C++-23-blue)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

## Table des matières
1. [Vue d'ensemble](#vue-densemble)
2. [Architecture du projet](#architecture-du-projet)
3. [Concepts scientifiques](#concepts-scientifiques)
4. [Pipeline algorithmique](#pipeline-algorithmique)
5. [Utilisation](#utilisation)
6. [Optimisations](#optimisations)

---

## Vue d'ensemble

Ce projet implémente un système de reconnaissance audio local inspiré de Shazam avec une interface graphique. Il permet d'identifier une chanson à partir d'un court extrait audio (5 secondes suffisent) en la comparant à une base de données de chansons indexées.

**Principe** : Transformer un signal audio en "empreintes digitales" (fingerprints) uniques, puis rechercher ces empreintes dans une base de données.

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

**Conversion en mono** : Moyenne des canaux (src/audio.cpp:69-84)
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

### 3. Transformée de Fourier Rapide (FFT)

La FFT transforme un signal temporel en signal fréquentiel.

**Principe** : Décompose le son en ses fréquences constituantes
```
Signal temporel (amplitude vs temps) → Signal fréquentiel (magnitude vs fréquence)
```

**Algorithme de Cooley-Tukey** (src/fft.cpp) :
- Divise récursivement le problème en deux
- Complexité : O(N log N) au lieu de O(N²)
- Nécessite que N soit une puissance de 2

### 4. Extraction des pics (Peaks)

Un **pic** est une fréquence localement maximale dans le spectre.

**Critères de sélection** :
1. Magnitude > seuil (threshold = 1% du max)
2. Plus grand que ses voisins immédiats
3. Garde seulement les N plus forts (top 5 par frame)

**Pourquoi ?** : Réduit le bruit et conserve uniquement les caractéristiques distinctives.

### 5. Fingerprinting (Empreintes audio)

Un **fingerprint** est un hash unique représentant une combinaison de pics.

**Algorithme** (src/fingerprint.cpp) :

Pour chaque pic "anchor" (ancre) :
1. Trouve les pics dans les frames suivantes (fenêtre de 5 frames, max 3 pics par frame)
2. Calcule un hash avec :
   - Fréquence du pic anchor
   - Fréquence du pic cible
   - Delta temporel entre eux

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

### Echantillonnage explication

4096 echantillons et hop de 2048 <br>
signal : <br>
audio : <-----------------------><br>
frame1: [4096]<br>
frame2:-----[4096]<br>
frame2:---------[4096]<br>

50% de chevauchement

### FingerPrint explication

exemple : Famout = 3, donc 3 paires par anchorPeak
   Frame    : A1, A2
   Frame +1 : B1, B2, B3 
   Frame +2 : C1, C2, C3
   Frame +3 : D1, D2, D3

AnchorPeak A1 :
   hash1 -> A1, B1, dt=1
   hash1 -> A1, B2, dt=1
   hash1 -> A1, B3, dt=1

AnchorPeak A2 :
   hash1 -> A2, B1, dt=1
   hash1 -> A2, B2, dt=1
   hash1 -> A2, B3, dt=1

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

---

## Structures de données

### Structure optimisée (v2.0) : Hashing multi-niveau

**Index des fingerprints** :
```cpp
std::unordered_map<uint64_t, std::unordered_map<int, std::vector<int>>> fingerprintIndex;
//                  hash      songId              timeOffsets
```

**Avantages** :
- Accès direct par chanson : O(1) au lieu de parcourir un vecteur
- Groupement logique : tous les offsets d'une chanson ensemble
- Meilleure performance : évite de parcourir des chansons non concernées

**Stockage des chansons** :
```cpp
std::map<int, Song> songs;
```
- Clé : ID de la chanson
- Valeur : métadonnées {id, title, artist, duration}

### Format de persistence (shazam.db)

```
[SONGS]
# id|title|artist|duration
1|Past Self|Sleep Token|234.5

[FINGERPRINTS]
# hash|songId|timeOffset
1234567890|1|42
9876543210|1|43
```

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

**Format audio supporté** : WAV 16-bit, mono ou stéréo, 44100 Hz (recommandé)

### Lancement

```bash
./shazam         # Linux/Mac
shazam.exe       # Windows
```

### Interface Graphique

L'application s'ouvre avec un **menu principal** offrant plusieurs options :

1. **Indexer /data/** : Scanne et indexe tous les WAV du dossier `/data`
2. **Ajouter un son** : Ajoute un seul fichier audio à la base de données
3. **Identifier fichier** : Ouvre un dialogue pour sélectionner un fichier à identifier
4. **Voir chansons** : Affiche la liste de toutes les chansons indexées
5. **Statistiques BDD** : Affiche les statistiques de la base de données
6. **Historique** : Affiche l'historique des recherches
7. **Vider BDD** : Supprime toutes les données indexées (avec confirmation)
8. **Quitter** : Ferme l'application

### Workflow typique

#### Première utilisation
1. Lance l'application
2. Clique sur **"Indexer /data/"**
3. Attends la fin de l'indexation (barre de progression)
4. Les chansons sont maintenant en base de données

#### Ajouter une chanson unique
1. Clique sur **"Ajouter un son"**
2. Clique sur **"Choisir un fichier audio"** et sélectionne ton fichier WAV
3. Clique sur **"Indexer le fichier"**
4. Une barre de progression détaillée s'affiche pendant l'indexation
5. Message de confirmation : "Chanson ajoutee avec succes: [Artiste] - [Titre]"

**Note** : Le format du nom doit être `Artiste_Titre.wav` pour l'extraction automatique des métadonnées.

#### Identifier une chanson
1. Clique sur **"Identifier fichier"**
2. Sélectionne un fichier WAV (même un court extrait de 5-10 secondes)
3. L'analyse se lance automatiquement
4. Les résultats s'affichent avec :
   - Titre et artiste
   - Score et confiance
   - "MATCH CONFIRME!" si confiance >= 70%

**Interprétation des résultats** :
- >= 70% : Match confirmé
- < 70% : "Aucun son trouvé" (la chanson n'est pas en base ou qualité insuffisante)

---

## Optimisations

### Optimisations Implémentées (v2.0)

1. **Recherche rapide** : Traite uniquement les 10 premières secondes pour les queries
2. **Cache de fingerprints** : Stocke le nombre de fingerprints par chanson pour calcul rapide de confiance
3. **Filtrage intelligent** : N'affiche que les matches >= 70% de confiance
4. **Interface réactive** : Mise à jour de l'UI toutes les 100 frames pendant l'indexation
5. **Structure multi-niveau** : Index hash -> songId -> offsets pour accès O(1) par chanson

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
Décembre 2025

**Technologies** :
- C++23
- CMake 3.20+
- SFML 2.6 (Graphics, Window)
- STL (unordered_map, vector, algorithms)
- FFT custom (Cooley-Tukey)