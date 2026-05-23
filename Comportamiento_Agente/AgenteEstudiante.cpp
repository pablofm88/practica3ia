#include "AgenteEstudiante.hpp"
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>

AgenteEstudiante::AgenteEstudiante(int id, int profundidadMax, double tiempoMax, int numHeuristica, ModoJuego modo) 
    : id(id), profundidadMax(profundidadMax), tiempoMaxSegundos(tiempoMax), numHeuristica(numHeuristica), modo(modo), abortarBanda(false) {
    nodosVisitados = 0;
}

bool AgenteEstudiante::tieneLimiteDeTiempo() const {
    return modo != ModoJuego::STATUS;
}

std::pair<int, int> AgenteEstudiante::think(const Tablero& tablero) {
    std::pair<int, int> mejor;
    nodosVisitados = 0;
    abortarBanda = false;
    inicioBusqueda = std::chrono::steady_clock::now();

    switch (modo)
    {
    case ModoJuego::ALEATORIO:
        return JuegaAleatorio(tablero);
        break;
    
    case ModoJuego::STATUS:
        Status(tablero, mejor);
        return mejor;
        break;    

    case ModoJuego::MINIMAX:
        minimax(tablero, 0, profundidadMax, mejor);
        return mejor;
        break; 

    case ModoJuego::INTELIGENTE:
        return JuegaInteligente(tablero);   
        break;
    }
        
    return {-1, -1};
}


/**
 * @brief Compara dos tableros para identificar cuál ha sido el movimiento realizado.
 * @param padre Estado inicial del tablero.
 * @param hijo Estado resultante tras un movimiento.
 * @return Un par (fila, columna) con la posición de la nueva pieza.
 */
std::pair<int, int> SacarMovimiento(const Tablero& padre, const Tablero &hijo){
    for(int f=0; f<padre.getFilas(); ++f)
        for(int c=0; c<padre.getColumnas(); ++c)
            if (padre.getCelda(f,c) == 0 && hijo.getCelda(f,c) != 0) 
                return {f, c};
    return {-1, -1};
}

/**
 * @brief Implementa un agente que juega de forma totalmente aleatoria.
 * @param tablero Estado actual del juego.
 * @return La jugada elegida al azar.
 */
std::pair<int, int> AgenteEstudiante::JuegaAleatorio(const Tablero& tablero) {

    // Calculo los tableros descendientes de tablero
    auto sucesores = tablero.getSucesores();

    // Si no tiene descendientes, paso el turno
    if (sucesores.empty()) return {-1, -1};

    // Elijo aleatoriamente uno de los descendientes
    int elegido = rand() % sucesores.size();

    // Saco el movimiento realizado comparando el tablero original con el elegido.
    std::pair<int,int> Mov = SacarMovimiento(tablero, sucesores[elegido]);

    return Mov;
}


/**
 * @brief Algoritmo de resolución completa para estados de final de juego.
 * Determina si una posición está matemáticamente ganada, perdida o empatada.
 * @param tablero Estado a evaluar.
 * @param Mov [Salida] La jugada óptima encontrada.
 * @return Resultado del análisis (VICTORIA, DERROTA o EMPATE).
 */
AgenteEstudiante::Resultado AgenteEstudiante::Status(const Tablero &tablero, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    Mov = {-1, -1};
    int ganador = tablero.comprobarGanador();
    int rival = (id == 1) ? 2 : 1;

    if (ganador == id) return Resultado::VICTORIA;
    if (ganador == rival) return Resultado::DERROTA;
    if (ganador == -1) return Resultado::EMPATE;

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return Resultado::EMPATE;

    bool maximizando = (tablero.getJugadorTurno() == id);
    Resultado mejor = maximizando ? Resultado::DERROTA : Resultado::VICTORIA;
    Mov = SacarMovimiento(tablero, sucesores[0]);

    for (const auto &hijo : sucesores) {
        std::pair<int,int> movHijo;
        Resultado res = Status(hijo, movHijo);
        std::pair<int,int> movActual = SacarMovimiento(tablero, hijo);

        if (maximizando) {
            if (res == Resultado::VICTORIA) {
                Mov = movActual;
                return Resultado::VICTORIA;
            }
            if (res == Resultado::EMPATE && mejor == Resultado::DERROTA) {
                mejor = Resultado::EMPATE;
                Mov = movActual;
            }
        } else {
            if (res == Resultado::DERROTA) {
                Mov = movActual;
                return Resultado::DERROTA;
            }
            if (res == Resultado::EMPATE && mejor == Resultado::VICTORIA) {
                mejor = Resultado::EMPATE;
                Mov = movActual;
            }
        }
    }

    return mejor;
}



/**
 * @brief Implementación del algoritmo Minimax clásico.
 * @param tablero Estado actual.
 * @param profundidad Nivel actual en el árbol de búsqueda.
 * @param prof_Max Límite de profundidad de la búsqueda.
 * @param Mov [Salida] La mejor jugada encontrada en la raíz.
 * @return Valor heurístico del estado.
 */
double AgenteEstudiante::minimax(const Tablero &tablero, int profundidad, int prof_Max, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    Mov = {-1, -1};
    int ganador = tablero.comprobarGanador();
    int rival = (id == 1) ? 2 : 1;

    if (ganador == id) return GANAR - profundidad;
    if (ganador == rival) return PERDER + profundidad;
    if (ganador == -1) return 0;
    if (profundidad >= prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    double mejorValor = maximizando ? MenosInfinito : MasInfinito;
    Mov = SacarMovimiento(tablero, sucesores[0]);

    for (const auto &hijo : sucesores) {
        std::pair<int,int> movHijo;
        double valor = minimax(hijo, profundidad + 1, prof_Max, movHijo);
        if (abortarBanda) return 0;

        std::pair<int,int> movActual = SacarMovimiento(tablero, hijo);

        if (maximizando) {
            if (valor > mejorValor) {
                mejorValor = valor;
                Mov = movActual;
            }
        } else {
            if (valor < mejorValor) {
                mejorValor = valor;
                Mov = movActual;
            }
        }
    }

    return mejorValor;
}


/**
 * @brief Punto de entrada para el juego inteligente.
 * @param tablero Estado actual del juego.
 * @return La jugada elegida por el algoritmo de búsqueda.
 */
std::pair<int, int> AgenteEstudiante::JuegaInteligente(const Tablero& tablero) {
    std::pair<int,int> Mov;

    double valor = alfaBeta(tablero, 0, profundidadMax, MenosInfinito, MasInfinito, Mov);
    std::cout << "Valor Minimax: " << valor << "\tJugada: (" << Mov.first << ", " << Mov.second << ")\n";
    return Mov;
}




/**
 * @brief Implementación del algoritmo Minimax con Poda Alfa-Beta.
 * @param tablero Estado actual.
 * @param profundidad Nivel actual en el árbol de búsqueda.
 * @param prof_Max Límite de profundidad de la búsqueda.
 * @param alfa Valor mínimo garantizado para el jugador MAX.
 * @param beta Valor máximo garantizado para el jugador MIN.
 * @param Mov [Salida] La mejor jugada encontrada en la raíz.
 * @return Valor heurístico del estado tras la poda.
 */
double AgenteEstudiante::alfaBeta(const Tablero &tablero, int profundidad, int prof_Max, double alfa, double beta, std::pair<int,int> &Mov) {
    /* ============== Este trozo de código se tiene que quedar aquí  =============== */
    nodosVisitados++;
    if (abortarBanda) return 0;
    
    if (std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    Mov = {-1, -1};
    int ganador = tablero.comprobarGanador();
    int rival = (id == 1) ? 2 : 1;

    if (ganador == id) return GANAR - profundidad;
    if (ganador == rival) return PERDER + profundidad;
    if (ganador == -1) return 0;
    if (profundidad >= prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    Mov = SacarMovimiento(tablero, sucesores[0]);

    if (maximizando) {
        double mejorValor = MenosInfinito;

        for (const auto &hijo : sucesores) {
            std::pair<int,int> movHijo;
            double valor = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (abortarBanda) return 0;

            if (valor > mejorValor) {
                mejorValor = valor;
                Mov = SacarMovimiento(tablero, hijo);
            }

            alfa = std::max(alfa, mejorValor);
            if (alfa >= beta) break;
        }

        return mejorValor;
    } else {
        double mejorValor = MasInfinito;

        for (const auto &hijo : sucesores) {
            std::pair<int,int> movHijo;
            double valor = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (abortarBanda) return 0;

            if (valor < mejorValor) {
                mejorValor = valor;
                Mov = SacarMovimiento(tablero, hijo);
            }

            beta = std::min(beta, mejorValor);
            if (alfa >= beta) break;
        }

        return mejorValor;
    }
}

/**
 * @brief Función heurística para evaluar la calidad de un tablero.
 * @param tablero Estado a evaluar.
 * @return Puntuación numérica (positiva para ventaja de J1, negativa para J2).
 */
double AgenteEstudiante::heuristica(const Tablero& tablero) {
    switch(numHeuristica) {
        case 0: return heuristicaPrueba(tablero);
                break;
        case 1: return heuristica1(tablero);
                break;
        case 2: return heuristica2(tablero);
                break;
        default: return heuristica1(tablero);
    }
}

double AgenteEstudiante::heuristicaPrueba(const Tablero& tablero) {
    // n es el número de fichas en línea para ganar.
    int n = tablero.getNParaGanar();
    int oponente = (id == 1) ? 2 : 1;
    double score_positivo = 0;

    double score_negativo = 0;

    for (int f=0; f< tablero.getFilas(); f++ ){
        for (int c = 0; c< tablero.getColumnas(); c++){
            if (tablero.getCelda(f,c) != 0 ){
                int valor = tablero.getFilas()-abs(f-(tablero.getFilas()/2)) + tablero.getColumnas()-abs(c-(tablero.getColumnas()/2)); 
                if (tablero.getCelda(f,c) == id){
                  score_positivo += valor;
                 }
                else {
                  score_negativo += valor;
                }
            }
        }
    }

   
    return score_positivo - score_negativo;
}


double AgenteEstudiante::heuristica1(const Tablero& tablero) {
    int rival = (id == 1) ? 2 : 1;
    int ganador = tablero.comprobarGanador();

    if (ganador == id) return GANAR;
    if (ganador == rival) return PERDER;
    if (ganador == -1) return 0;

    int filas = tablero.getFilas();
    int columnas = tablero.getColumnas();
    int n = tablero.getNParaGanar();
    const int df[4] = {0, 1, 1, 1};
    const int dc[4] = {1, 0, 1, -1};

    std::vector<double> pesos(n + 1, 0.0);
    for (int i = 1; i <= n; ++i) pesos[i] = (i == 1) ? 1.0 : pesos[i - 1] * 8.0;

    double valor = 0.0;

    for (int f = 0; f < filas; ++f) {
        for (int c = 0; c < columnas; ++c) {
            for (int d = 0; d < 4; ++d) {
                int finF = f + df[d] * (n - 1);
                int finC = c + dc[d] * (n - 1);
                if (finF < 0 || finF >= filas || finC < 0 || finC >= columnas) continue;

                int mias = 0, suyas = 0;
                double bonus = 0.0;

                for (int i = 0; i < n; ++i) {
                    int nf = f + df[d] * i;
                    int nc = c + dc[d] * i;
                    int celda = tablero.getCelda(nf, nc);

                    if (celda == id) mias++;
                    else if (celda == rival) suyas++;
                    else {
                        auto tipo = tablero.getTipoCelda(nf, nc);
                        if (tipo == Tablero::TipoCelda::VERDE) bonus += 2.0;
                        else if (tipo == Tablero::TipoCelda::ROJO) bonus -= 2.0;
                        else if (tipo == Tablero::TipoCelda::AMARILLO) bonus -= 1.0;
                    }
                }

                if (mias > 0 && suyas == 0) valor += pesos[mias] + bonus;
                else if (suyas > 0 && mias == 0) valor -= pesos[suyas] + bonus;
            }
        }
    }

    valor += 120.0 * (tablero.contarCombinaciones(n - 1, id) - tablero.contarCombinaciones(n - 1, rival));
    if (n > 2) valor += 25.0 * (tablero.contarCombinaciones(n - 2, id) - tablero.contarCombinaciones(n - 2, rival));

    for (int f = 0; f < filas; ++f) {
        for (int c = 0; c < columnas; ++c) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;

            double pos = (filas - std::abs(f - filas / 2)) + (columnas - std::abs(c - columnas / 2));
            if (celda == id) valor += pos;
            else valor -= pos;
        }
    }

    return valor;
}

double AgenteEstudiante::heuristica2(const Tablero& tablero) {
    int rival = (id == 1) ? 2 : 1;
    int ganador = tablero.comprobarGanador();
    int n = tablero.getNParaGanar();

    if (ganador == id) return GANAR;
    if (ganador == rival) return PERDER;
    if (ganador == -1) return 0;

    double valor = heuristicaPrueba(tablero);
    valor += 80.0 * (tablero.contarCombinaciones(n - 1, id) - tablero.contarCombinaciones(n - 1, rival));
    if (n > 2) valor += 15.0 * (tablero.contarCombinaciones(n - 2, id) - tablero.contarCombinaciones(n - 2, rival));

    return valor;
}

