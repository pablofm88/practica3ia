#include "AgenteEstudiante.hpp"
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
    const long long LIMITE_NODOS_MINIMAX = 150000;
    const long long LIMITE_NODOS_ALFABETA = 900000;
}

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
    mejorMovimientoH = mejorMovimientoRapido(tablero);
    mejor = mejorMovimientoH;

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
        return (mejor.first == -1) ? mejorMovimientoH : mejor;
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

bool AgenteEstudiante::esTerminal(const Tablero& tablero) const {
    return tablero.comprobarGanador() != 0 || tablero.estaLleno();
}

bool AgenteEstudiante::tiempoAgotado() const {
    if (tiempoMaxSegundos <= 0) return false;
    double usado = std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count();
    return usado >= tiempoMaxSegundos;
}

std::pair<int, int> AgenteEstudiante::mejorMovimientoRapido(const Tablero& tablero) {
    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return {-1, -1};

    bool maximizando = (tablero.getJugadorTurno() == id);
    double mejorValor = maximizando ? MenosInfinito : MasInfinito;
    std::pair<int, int> mejor = SacarMovimiento(tablero, sucesores[0]);

    for (const auto &hijo : sucesores) {
        double valor = heuristica(hijo);
        if ((maximizando && valor > mejorValor) || (!maximizando && valor < mejorValor)) {
            mejorValor = valor;
            mejor = SacarMovimiento(tablero, hijo);
        }
    }

    return mejor;
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
    int rival = (id == 1) ? 2 : 1;
    Mov = {-1, -1};

    if (tablero.getFilas() * tablero.getColumnas() > 16) {
        Mov = mejorMovimientoRapido(tablero);
        return Resultado::EMPATE;
    }

    std::function<Resultado(const Tablero&, int)> resolver = [&](const Tablero &estado, int nivel) -> Resultado {
        nodosVisitados++;

        int ganador = estado.comprobarGanador();
        if (ganador == id) return Resultado::VICTORIA;
        if (ganador == rival) return Resultado::DERROTA;
        if (ganador == -1) return Resultado::EMPATE;

        auto sucesores = estado.getSucesores();
        if (sucesores.empty()) return Resultado::EMPATE;

        bool maximizando = (estado.getJugadorTurno() == id);
        Resultado mejor = maximizando ? Resultado::DERROTA : Resultado::VICTORIA;

        for (const auto &hijo : sucesores) {
            Resultado actual = resolver(hijo, nivel + 1);

            if (maximizando) {
                if (actual == Resultado::VICTORIA) {
                    if (nivel == 0) Mov = SacarMovimiento(estado, hijo);
                    return Resultado::VICTORIA;
                }
                if (actual == Resultado::EMPATE && mejor == Resultado::DERROTA) {
                    mejor = Resultado::EMPATE;
                    if (nivel == 0) Mov = SacarMovimiento(estado, hijo);
                }
                else if (mejor == Resultado::DERROTA && nivel == 0 && Mov.first == -1) {
                    Mov = SacarMovimiento(estado, hijo);
                }
            } else {
                if (actual == Resultado::DERROTA) {
                    if (nivel == 0) Mov = SacarMovimiento(estado, hijo);
                    return Resultado::DERROTA;
                }
                if (actual == Resultado::EMPATE && mejor == Resultado::VICTORIA) {
                    mejor = Resultado::EMPATE;
                    if (nivel == 0) Mov = SacarMovimiento(estado, hijo);
                }
                else if (mejor == Resultado::VICTORIA && nivel == 0 && Mov.first == -1) {
                    Mov = SacarMovimiento(estado, hijo);
                }
            }
        }

        return mejor;
    };

    return resolver(tablero, 0);
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
    if (abortarBanda) return heuristica(tablero);
    
    if (tiempoAgotado()) {
        abortarBanda = true;
        return heuristica(tablero);
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    if (nodosVisitados > LIMITE_NODOS_MINIMAX) {
        abortarBanda = true;
        return heuristica(tablero);
    }

    int ganador = tablero.comprobarGanador();
    int rival = (id == 1) ? 2 : 1;

    if (ganador == id) return GANAR - profundidad;
    if (ganador == rival) return PERDER + profundidad;
    if (ganador == -1) return 0;
    if (profundidad >= prof_Max || esTerminal(tablero)) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    double mejorValor = maximizando ? MenosInfinito : MasInfinito;
    if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, sucesores[0]);

    for (const auto &hijo : sucesores) {
        std::pair<int,int> movHijo;
        double valor = minimax(hijo, profundidad + 1, prof_Max, movHijo);
        if (abortarBanda) {
            if (profundidad == 0) mejorMovimientoH = Mov;
            return mejorValor;
        }

        if (maximizando) {
            if (valor > mejorValor) {
                mejorValor = valor;
                if (profundidad == 0) {
                    Mov = SacarMovimiento(tablero, hijo);
                    mejorMovimientoH = Mov;
                }
            }
        } else {
            if (valor < mejorValor) {
                mejorValor = valor;
                if (profundidad == 0) {
                    Mov = SacarMovimiento(tablero, hijo);
                    mejorMovimientoH = Mov;
                }
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
    std::pair<int,int> Mov = mejorMovimientoRapido(tablero);
    mejorMovimientoH = Mov;

    double valor = alfaBeta(tablero, 0, profundidadMax, MenosInfinito, MasInfinito, Mov);
    if (Mov.first == -1) Mov = mejorMovimientoH;
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
    if (abortarBanda) return heuristica(tablero);
    
    if (tiempoAgotado()) {
        abortarBanda = true;
        return heuristica(tablero);
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    if (nodosVisitados > LIMITE_NODOS_ALFABETA) {
        abortarBanda = true;
        return heuristica(tablero);
    }

    int ganador = tablero.comprobarGanador();
    int rival = (id == 1) ? 2 : 1;

    if (ganador == id) return GANAR - profundidad;
    if (ganador == rival) return PERDER + profundidad;
    if (ganador == -1) return 0;
    if (profundidad >= prof_Max || esTerminal(tablero)) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    if (sucesores.size() > 1) {
        std::stable_sort(sucesores.begin(), sucesores.end(),
            [&](const Tablero &a, const Tablero &b) {
                double va = heuristica(a);
                double vb = heuristica(b);
                return maximizando ? (va > vb) : (va < vb);
            });
    }

    if (profundidad == 0 && Mov.first == -1) Mov = SacarMovimiento(tablero, sucesores[0]);

    if (maximizando) {
        double mejorValor = MenosInfinito;

        for (const auto &hijo : sucesores) {
            std::pair<int,int> movHijo;
            double valor = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (abortarBanda) {
                if (profundidad == 0) mejorMovimientoH = Mov;
                return mejorValor;
            }

            if (valor > mejorValor) {
                mejorValor = valor;
                if (profundidad == 0) {
                    Mov = SacarMovimiento(tablero, hijo);
                    mejorMovimientoH = Mov;
                }
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
            if (abortarBanda) {
                if (profundidad == 0) mejorMovimientoH = Mov;
                return mejorValor;
            }

            if (valor < mejorValor) {
                mejorValor = valor;
                if (profundidad == 0) {
                    Mov = SacarMovimiento(tablero, hijo);
                    mejorMovimientoH = Mov;
                }
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
    double pesos[10] = {0.0, 2.0, 12.0, 60.0, 400.0, 5000.0, 20000.0, 50000.0, 100000.0, 200000.0};
    double valor = 0.0;

    for (int f = 0; f < filas; ++f) {
        for (int c = 0; c < columnas; ++c) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;

            double centro = (filas - std::abs(f - filas / 2)) + (columnas - std::abs(c - columnas / 2));
            if (celda == id) valor += centro;
            else valor -= centro;
        }
    }

    for (int f = 0; f < filas; ++f) {
        for (int c = 0; c < columnas; ++c) {
            for (int d = 0; d < 4; ++d) {
                int finF = f + (n - 1) * df[d];
                int finC = c + (n - 1) * dc[d];
                if (finF < 0 || finF >= filas || finC < 0 || finC >= columnas) continue;

                int mias = 0, suyas = 0, libres = 0;

                for (int k = 0; k < n; ++k) {
                    int nf = f + k * df[d];
                    int nc = c + k * dc[d];
                    int celda = tablero.getCelda(nf, nc);

                    if (celda == id) mias++;
                    else if (celda == rival) suyas++;
                    else libres++;
                }

                if (mias > 0 && suyas == 0) {
                    valor += pesos[mias];
                    if (mias == n - 1 && libres == 1) valor += 2000.0;
                } else if (suyas > 0 && mias == 0) {
                    valor -= pesos[suyas];
                    if (suyas == n - 1 && libres == 1) valor -= 2500.0;
                }
            }
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

