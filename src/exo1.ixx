module;
// Import rapide de la bibliothèque standard
import std;

// Nommage et exportation de ce module
export module exo1;

// On met toutes les classes du module dans un espace de nom
// ayant le même nom que le module pour plus de clarté et éviter
// les conflits de nommage.
// On exporte la totalité des classe de l'espace de nom.
export namespace exo1 {

    // Exemple de classe pour convention de nommage
    class MyClass {
    public:
        MyClass() = default;

        explicit MyClass(const double value): value{value} {}

        // Ne pas oublier d'être "const-correct"
        double GetValue() const { return value; }

        void SetValue(const double value) { this->value = value; }

        std::string ToString() const {
            return std::format("{0:.2f}", value);
        }

    private:
        // Prendre l'habitude d'utiliser la syntaxe moderne d'initialisation
        // identique pour tous les types de données : simples, class, struct, ...
        double value{0.0};
    };

    // Exemple de classe utilisée dans la fonction main()
    class Application {
    public:
        // Tout le code de l'exercice doit être dans ce constructeur
        Application() {
            std::cout << "Hello from exo1::Application()\n";
            // Par défaut les variables sont constantes et on utilise l'inférence de type avec "auto"
            // ainsi que la syntaxe moderne d'instanciation automatique et d'initialisation avec {}
            const auto obj1 = MyClass{123.456};
            std::cout << obj1.ToString() << std::endl;
        }
    };

}