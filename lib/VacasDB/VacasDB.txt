#include "VacasDB.h"

const double racion = 1.0;
const uint32_t intervaloMinimoEntreRaciones_seg = 600;

bool VacasDB::open(void){
    if(db_)
        return true;

    if(sqlite3_open(DB_PATH, &db_) != SQLITE_OK){
        db_ = nullptr;
        return false;
    }
    
    return true;
}

void VacasDB::end(){
    if(db_){
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

/**
 * @brief Verifica si la ultima fecha de actualización del valor comidoHoy corresponde al dia actual. Si no lo es, actualiza comidoHoy a cero
 * 
 * @param int numero de identificación de la vaca
 * @return 0 si estaba OK, 1 si hubo error, 2 si hubo que resetear
 */

int VacasDB::asegurarComidoHoy(int idVaca) {
    time_t hoy;
    if(!obtenerFechaHoyUTC(&hoy))
        return 1;

    const char* sql_select = "SELECT fecha_ultima_comida, comido_hoy FROM Vacas WHERE id_vaca = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql_select, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 1;
    }

    sqlite3_bind_int(stmt, 1, idVaca);  // PIDX: The parameter index. The first parameter has an index of one (1).

    time_t fechaUltimaComida = 0;
    bool necesitaReset = false;

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {                                         // la vaca existe
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {          // fecha_ultima_comida no es NULL -> alguna vez comio
            fechaUltimaComida = (time_t)sqlite3_column_int64(stmt, 0);

            if (!mismoDia(fechaUltimaComida, hoy)) {
                necesitaReset = true;
            }
        } else {            // la vaca nunca comio
            necesitaReset = true;
        }
    } else {
        // La vaca no existe (para futura mejora)
        // No ejecuta necesitaReset pero se podria dejar explicito el sqlite3_finalize(stmt) y return
    }

    sqlite3_finalize(stmt);

    if(necesitaReset){
        const char* sql_update = "UPDATE Vacas SET comido_hoy = 0, fecha_ultima_comida = ? WHERE id_vaca = ?;";

        sqlite3_stmt* upd = nullptr;
        rc = sqlite3_prepare_v2(db_, sql_update, -1, &upd, nullptr);
        if (rc != SQLITE_OK) {
            return 1;
        }

        sqlite3_bind_int64(upd, 1, (sqlite3_int64)hoy);
        sqlite3_bind_int(upd, 2, idVaca);
        sqlite3_step(upd);
        sqlite3_finalize(upd);
        return 2;
    }
    
    return 0;
}

/****************************** PUBLICAS  ******************************/

/**
 * @brief Valida que 1) la vaca haya comido menos que su cantidad diaria asignada y 2) que la ultima vez que comió haya sido hace un tiempo mayor al intervalo mínimo definido
 * 
 * PRECONDICIÓN:
 * - la función que ejecute la alimentación sería la encargada de saber cuánto debe dar
 * - se asume que racion es múltiplo de cantidadDiaria, con lo cual si comido_hoy < cantidadDiaria entonces existe al menos una ración completa disponible, y no hay que validar que comido_hoy + racion <= cantidadDiaria
 * 
 * 
 * @param int numero de identificación de la vaca
 * @return true si se cumplen las condiciones, false en caso contrario
 */
bool VacasDB::puedeComer(int idVaca){
    int res = asegurarComidoHoy(idVaca);
    if(res == 1)
        return false;

    bool puedeComer = false;

    double cantidadComioHoy = 0;
    double cantidadDiaria = 0;
    time_t fechaUltimaComida = 0;
    
    const char* sql_select = "SELECT cantidad_diaria, comido_hoy, fecha_ultima_comida FROM Vacas WHERE id_vaca = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql_select, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, idVaca);
    rc = sqlite3_step(stmt);

    if(rc == SQLITE_ROW) {   // LA VACA EXISTE
        // No verifico si alguna vez comio porque ya lo hace asegurarComioHoy
        cantidadDiaria = sqlite3_column_double(stmt, 0);
        cantidadComioHoy = sqlite3_column_double(stmt, 1);
        fechaUltimaComida = (time_t) sqlite3_column_int64(stmt, 2);
        time_t hoy;

        if(!obtenerFechaHoyUTC(&hoy)){
            sqlite3_finalize(stmt);
            return false;               // Mejora: manejar error
        }


        if(res == 2 && cantidadComioHoy < cantidadDiaria){   // No verifico fecha porque la acabo de actualizar, solo tengo que ver que cantidadDiaria no sea 0
            puedeComer = true;
        } else if(res != 2 && cantidadComioHoy < cantidadDiaria && hoy - fechaUltimaComida >= intervaloMinimoEntreRaciones_seg)
            puedeComer = true;
    }

    sqlite3_finalize(stmt);
    return puedeComer;
}

/**
 * @brief Registra una nueva comida
 * 
 * @param int numero de identificación de la vaca
 * @param double cantidad comida
 * @return true si se registro con exito, false en caso contrario
 */
bool VacasDB::registrarComida(int idVaca, double comido){
    time_t hoy;
    
    if(!obtenerFechaHoyUTC(&hoy))
        return false;

    const char* sql_insert = "INSERT INTO RegistroAlimentacion (id_vaca, fecha_hora, cantidad) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql_insert, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, idVaca);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)hoy);
    sqlite3_bind_double(stmt, 3, comido);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) 
        return false;

    const char* sql_update = "UPDATE Vacas SET comido_hoy = comido_hoy + ?, fecha_ultima_comida = ? WHERE id_vaca = ?;";
    
    sqlite3_stmt* upd = nullptr;
    rc = sqlite3_prepare_v2(db_, sql_update, -1, &upd, nullptr);
    if (rc != SQLITE_OK){
        // evaluar si se deberia eliminar de la otra tabla
        return false;
    }

    sqlite3_bind_double(upd, 1, comido);
    sqlite3_bind_int64(upd, 2, (sqlite3_int64)hoy);
    sqlite3_bind_int(upd, 3, idVaca);

    rc = sqlite3_step(upd);
    sqlite3_finalize(upd);

    if(rc == SQLITE_DONE){
        // evaluar si se deberia eliminar de la otra tabla
        return false;
    }
    
    return true;
}

/**
 * @brief Obtiene la sumatoria de todas las comidas en una fecha
 * 
 * @param int numero de identificación de la vaca
 * @param time_t fecha solicitada
 * @return Sumatoria de comidas
 */
double VacasDB::obtenerComidaEnFecha(int idVaca, time_t fecha){
    tm t;
    localtime_r(&fecha, &t);
    t.tm_hour = 0;
    t.tm_min  = 0;
    t.tm_sec  = 0;

    time_t inicioDia = mktime(&t);
    time_t finDia = inicioDia + 24 * 60 * 60;

    const char* sql_select =
        "SELECT COALESCE(SUM(cantidad), 0) FROM RegistroAlimentacion WHERE id_vaca = ? AND fecha_hora >= ? AND fecha_hora < ?;";

    sqlite3_stmt* stmt = nullptr;
    double cantidadComida = 0.0;

    int rc = sqlite3_prepare_v2(db_, sql_select, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0.0; // revisar
    }

    sqlite3_bind_int(stmt, 1, idVaca);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)inicioDia);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)finDia);

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW) {   // Existe la vaca y la fecha
        cantidadComida = sqlite3_column_double(stmt, 0);
    } 

    // Si no existe la vaca o no existe la fecha, cantidadComida permanece 0.0

    sqlite3_finalize(stmt);
    return cantidadComida;
}

/**
 * @brief Actualiza la cantidad diaria de comida asignada a una vaca
 * 
 * @param int numero de identificación de la vaca
 * @param double nueva cantidad
 * @return true si se realizó con éxito, false en caso contrario
 */
bool VacasDB::actualizarCantidadDiaria(int idVaca, double nuevaCantidad){
    const char* sql_update = "UPDATE Vacas SET cantidad_diaria = ? WHERE id_vaca = ?;";

    sqlite3_stmt* upd = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql_update, -1, &upd, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_double(upd, 1, nuevaCantidad);
    sqlite3_bind_int(upd, 2, idVaca);
    
    rc = sqlite3_step(upd);

    sqlite3_finalize(upd);

    if (rc != SQLITE_DONE)
        return false;

    return sqlite3_changes(db_) == 1; // si es distinto de 1 (0) no se actualizó ninguna fila
}

bool VacasDB::registrarVaca(int idVaca, double cantidadDiaria){
    const char* sql_insert = "INSERT INTO Vacas (id_vaca, cantidad_diaria, comido_hoy, fecha_ultima_comida) VALUES (?, ?, 0, NULL);";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql_insert, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, idVaca);
    sqlite3_bind_double(stmt, 2, cantidadDiaria);    

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}