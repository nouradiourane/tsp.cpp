# TSP Solver - Heuristique 2-Opt

Ce projet implémente un solveur pour le **Problème du Voyageur de Commerce (TSP)** en C++. 
Il utilise l'algorithme d'optimisation locale **2-Opt** combiné à une approche **Multi-start**.

## Fonctions clés
- **Parsing :** Lecture de fichiers au format `.tsp` (TSPLIB).
- **Algorithme :** Génération de tours aléatoires et optimisation par décroisement d'arêtes (2-Opt).
- **Gestion Mémoire :** Allocation dynamique optimisée pour de grandes instances.

  ##  Langages et Outils
- **Langage :** C++ (Standard 11/14)
- **Bibliothèques :** STL (`fstream`, `cmath`, `algorithm`, `string`)
- **Format de données :** Support des instances TSPLIB (EDGE_WEIGHT_SECTION et NODE_COORD_SECTION)

## Comment l'utiliser
1. Compilez le code : `g++ -O3 main.cpp -o tsp_solver`
2. Lancez l'exécutable : `./tsp_solver`
3. Entrez le nom de votre fichier (ex: `berlin52.tsp`).

## Résultats
Le programme génère un fichier `.tour` compatible avec les outils de visualisation TSPLIB.
