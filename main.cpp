#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <ctime>
#include <climits>
#include <algorithm> // Pour std::swap et std::stoi

using namespace std;

// --- STRUCTURES DE DONNÉES ---

/**
 * Structure stockant les données du problème TSP lues depuis un fichier .tsp
 */
struct DonneesTSP {
    int dimension;            // Nombre de villes
    double** distances;       // Matrice d'adjacence (distances entre chaque ville)
    double** coord;           // Coordonnées (x, y) pour les types EUC_2D / ATT
    string nomInstance;       // Nom de l'instance
    string edgeWeightType;    // Type de calcul des poids (EUC_2D, ATT, etc.)
};

/**
 * Structure représentant une solution (un tour)
 */
struct Solution {
    int* chemin;              // Tableau d'indices des villes (taille dimension + 1 pour boucler)
    double distanceTotale;    // Coût total du cycle
};

// --- GESTION DE LA MÉMOIRE ---

void initialiserDonnees(DonneesTSP* data) {
    data->dimension = 0;
    data->distances = nullptr;
    data->coord = nullptr;
    data->nomInstance = "";
    data->edgeWeightType = "";
}

/**
 * Allocation dynamique des matrices pour les distances et coordonnées
 */
void allouerMemoire(DonneesTSP* data) {
    data->distances = new double*[data->dimension];
    for (int i = 0; i < data->dimension; i++) {
        data->distances[i] = new double[data->dimension];
        for (int j = 0; j < data->dimension; j++) {
            data->distances[i][j] = 0.0;
        }
    }

    data->coord = new double*[data->dimension];
    for (int i = 0; i < data->dimension; i++) {
        data->coord[i] = new double[2];
    }
}

void libererMemoire(DonneesTSP* data) {
    if (data->distances) {
        for (int i = 0; i < data->dimension; i++) {
            delete[] data->distances[i];
        }
        delete[] data->distances;
    }

    if (data->coord) {
        for (int i = 0; i < data->dimension; i++) {
            delete[] data->coord[i];
        }
        delete[] data->coord;
    }
}

// --- LECTURE ET PARSING ---

/**
 * Lit un fichier au format TSPLIB (.tsp)
 * Supporte les formats EDGE_WEIGHT_SECTION et NODE_COORD_SECTION
 */
bool lireFichierTSP(const string& nomFichier, DonneesTSP* data) {
    ifstream fichier(nomFichier);
    if (!fichier) {
        cerr << "Erreur : Impossible d'ouvrir le fichier " << nomFichier << endl;
        return false;
    }

    initialiserDonnees(data);
    string ligne;
    bool sectionPoids = false;
    bool sectionCoord = false;

    // Phase 1 : Lecture des en-têtes
    while (getline(fichier, ligne)) {
        if (ligne.find("DIMENSION") != string::npos) {
            data->dimension = stoi(ligne.substr(ligne.find(":") + 1));
            allouerMemoire(data);
        }
        else if (ligne.find("EDGE_WEIGHT_TYPE") != string::npos) {
            data->edgeWeightType = ligne.substr(ligne.find(":") + 1);
            // Nettoyage des espaces
            data->edgeWeightType.erase(0, data->edgeWeightType.find_first_not_of(" \t"));
        }
        else if (ligne.find("NAME") != string::npos) {
            data->nomInstance = ligne.substr(ligne.find(":") + 1);
        }
        else if (ligne.find("EDGE_WEIGHT_SECTION") != string::npos) {
            sectionPoids = true;
            break; 
        }
        else if (ligne.find("NODE_COORD_SECTION") != string::npos) {
            sectionCoord = true;
            break;
        }
    }

    // Phase 2 : Lecture des données numériques
    if (sectionPoids) {
        for (int i = 0; i < data->dimension - 1; i++) {
            for (int j = i + 1; j < data->dimension; j++) {
                fichier >> data->distances[i][j];
                data->distances[j][i] = data->distances[i][j];
            }
        }
    }
    else if (sectionCoord) {
        for (int i = 0; i < data->dimension; i++) {
            int id;
            fichier >> id >> data->coord[i][0] >> data->coord[i][1];
        }

        // Calcul de la matrice de distances à partir des coordonnées
        for (int i = 0; i < data->dimension; i++) {
            for (int j = i + 1; j < data->dimension; j++) {
                double dx = data->coord[i][0] - data->coord[j][0];
                double dy = data->coord[i][1] - data->coord[j][1];
                double dij;

                if (data->edgeWeightType == "ATT") { // Pseudo-Euclidienne (norme spécifique)
                    double rij = sqrt((dx * dx + dy * dy) / 10.0);
                    int tij = (int)(rij + 0.5);
                    dij = (tij < rij) ? tij + 1 : tij;
                } else {
                    dij = sqrt(dx * dx + dy * dy); // Euclidienne classique
                }
                data->distances[i][j] = data->distances[j][i] = dij;
            }
        }
    }

    fichier.close();
    return true;
}

// --- ALGORITHMES D'OPTIMISATION ---

/**
 * Génère un cycle Hamiltonien aléatoire (Shuffling)
 */
void genererSolutionAleatoire(Solution* sol, int dimension) {
    sol->chemin = new int[dimension + 1];
    for (int i = 0; i < dimension; i++) {
        sol->chemin[i] = i;
    }

    // Algorithme de Fisher-Yates pour le mélange
    for (int i = dimension - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap(sol->chemin[i], sol->chemin[j]);
    }

    // On ferme le cycle
    sol->chemin[dimension] = sol->chemin[0];
}

void calculerDistance(Solution* sol, DonneesTSP* data) {
    sol->distanceTotale = 0.0;
    for (int i = 0; i < data->dimension; i++) {
        sol->distanceTotale += data->distances[sol->chemin[i]][sol->chemin[i+1]];
    }
}

/**
 * Optimisation locale 2-Opt : améliore le tour en inversant des segments
 * pour supprimer les croisements de chemins.
 */
void optimisation2Opt(Solution* sol, DonneesTSP* data) {
    bool amelioration;
    do {
        amelioration = false;
        for (int i = 1; i < data->dimension - 1; i++) {
            for (int j = i + 1; j < data->dimension; j++) {
                // Calcul du gain potentiel sans recalculer tout le tour (gain de perf)
                double gain = data->distances[sol->chemin[i-1]][sol->chemin[j]] 
                            + data->distances[sol->chemin[i]][sol->chemin[j+1]]
                            - data->distances[sol->chemin[i-1]][sol->chemin[i]] 
                            - data->distances[sol->chemin[j]][sol->chemin[j+1]];

                if (gain < -0.0001) { // Si on réduit la distance
                    // Inversion du segment [i, j]
                    int debut = i, fin = j;
                    while (debut < fin) {
                        swap(sol->chemin[debut], sol->chemin[fin]);
                        debut++;
                        fin--;
                    }
                    sol->distanceTotale += gain;
                    amelioration = true;
                }
            }
        }
    } while (amelioration);
}

// --- UTILITAIRES DE SORTIE ---

void afficherSolution(Solution* sol, int dimension) {
    cout << "\nMeilleure solution trouvee (Distance = " << sol->distanceTotale << "):" << endl;
    for (int i = 0; i <= dimension; i++) {
        cout << sol->chemin[i] + 1;
        if (i < dimension) cout << " -> ";
    }
    cout << endl;
}

void sauvegarderSolution(Solution* sol, DonneesTSP* donnees, const string& nomFichier) {
    ofstream fichier(nomFichier);
    if (!fichier) {
        cerr << "Erreur : impossible de sauvegarder la solution." << endl;
        return;
    }

    fichier << "NAME : " << donnees->nomInstance << ".tour" << endl;
    fichier << "TYPE : TOUR" << endl;
    fichier << "DIMENSION : " << donnees->dimension << endl;
    fichier << "TOUR_SECTION" << endl;

    for (int i = 0; i < donnees->dimension; i++) {
        fichier << sol->chemin[i] + 1 << endl;
    }

    fichier << "-1" << endl << "EOF" << endl;
    fichier.close();
    cout << "\nSolution sauvegardee avec succes dans : " << nomFichier << endl;
}

void libererSolution(Solution* sol) {
    if (sol->chemin != nullptr) {
        delete[] sol->chemin;
        sol->chemin = nullptr;
    }
}

/**
 * Duplique une solution source vers une destination
 */
void copierSolution(Solution* dest, const Solution* src, int dimension) {
    if (src->chemin == nullptr) return;
    dest->chemin = new int[dimension + 1];
    for (int i = 0; i <= dimension; i++) {
        dest->chemin[i] = src->chemin[i];
    }
    dest->distanceTotale = src->distanceTotale;
}

// --- POINT D'ENTRÉE ---

int main() {
    srand(static_cast<unsigned int>(time(NULL)));
    
    DonneesTSP data;
    initialiserDonnees(&data);

    string nomFichier;
    cout << "--- Solveur TSP (2-Opt Multi-Essais) ---" << endl;
    cout << "Nom du fichier .tsp : ";
    cin >> nomFichier;

    if (!lireFichierTSP(nomFichier, &data)) {
        return 1;
    }

    if (data.dimension <= 0) {
        cerr << "Erreur : Dimension invalide." << endl;
        libererMemoire(&data);
        return 1;
    }

    Solution meilleureSolution;
    meilleureSolution.chemin = nullptr;
    meilleureSolution.distanceTotale = 1e18; // Valeur très élevée

    // L'approche "Multistart" : on lance l'optimisation depuis plusieurs points de départ aléatoires
    const int NB_ESSAIS = 1000; 
    cout << "Calcul en cours (Essais : " << NB_ESSAIS << ")..." << endl;

    for (int essai = 0; essai < NB_ESSAIS; essai++) {
        Solution solCourante;
        genererSolutionAleatoire(&solCourante, data.dimension);
        calculerDistance(&solCourante, &data);
        optimisation2Opt(&solCourante, &data);

        if (solCourante.distanceTotale < meilleureSolution.distanceTotale) {
            // Important : On libère l'ancienne meilleure avant de copier
            libererSolution(&meilleureSolution);
            copierSolution(&meilleureSolution, &solCourante, data.dimension);
        }

        libererSolution(&solCourante);
    }

    // Restitution des résultats
    afficherSolution(&meilleureSolution, data.dimension);
    string nomFichierSolution = nomFichier + ".tour";
    sauvegarderSolution(&meilleureSolution, &data, nomFichierSolution);

    // Nettoyage final
    libererSolution(&meilleureSolution);
    libererMemoire(&data);

    return 0;
}
