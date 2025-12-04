//import exo1;
#include "../include/gui.h"
#include "../include/database.h"
#include <iostream>

int main() {
    try {
        database::Database db("shazam.db");
        gui::GUI app(db);
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "\nErreur fatale: " << e.what() << "\n";
        return 1;
    }

    return 0;
}