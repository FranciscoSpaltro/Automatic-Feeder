#ifndef VACAS_DB_H
#define VACAS_DB_H

#include <sqlite3.h>
#include <time.h>
#include "TimeHandler.h"

#define DB_PATH "baseDatos"

class VacasDB {
    private:
        sqlite3* db_ = nullptr;

        bool open();
        void end();
        int asegurarComidoHoy(int idVaca);     // Anticipa el obtenerComidoHoy para resetear el contador si la fecha es anterior a hoy


    public:
        bool puedeComer(int idVaca);
        bool registrarComida(int idVaca, double comido);
        double obtenerComidaEnFecha(int idVaca, time_t fecha);
        bool actualizarCantidadDiaria(int idVaca, double nuevaCantidad);
        bool registrarVaca(int idVaca, double cantidadDiaria);

        ~VacasDB(){ end(); };

};

#endif  // VACAS_DB_H