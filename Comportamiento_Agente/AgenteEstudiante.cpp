#include "AgenteEstudiante.hpp"
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
    int rivalDe(int jugador) {
        return (jugador == 1) ? 2 : 1;
    }

    double valorTerminal(const Tablero& tablero, int id, int profundidad) {
        int ganador = tablero.comprobarGanador();
        if (ganador == id) return AgenteEstudiante::GANAR - profundidad;
        if (ganador == rivalDe(id)) return AgenteEstudiante::PERDER + profundidad;
        return 0.0;
    }

    int valorStatus(AgenteEstudiante::Resultado resultado) {
        if (resultado == AgenteEstudiante::Resultado::VICTORIA) return 1;
        if (resultado == AgenteEstudiante::Resultado::EMPATE) return 0;
        return -1;
    }

    double pesoLinea(int piezas) {
        static const double pesos[] = {0.0, 4.0, 35.0, 350.0, 6000.0, 200000.0, 800000.0, 2000000.0, 5000000.0, 10000000.0};
        if (piezas < 0) return 0.0;
        int numPesos = static_cast<int>(sizeof(pesos) / sizeof(pesos[0]));
        if (piezas < numPesos) return pesos[piezas];
        return pesos[numPesos - 1] * piezas;
    }

    double evaluarVentanas(const Tablero& tablero, int id) {
        int rival = rivalDe(id);
        int filas = tablero.getFilas();
        int columnas = tablero.getColumnas();
        int n = tablero.getNParaGanar();
        const int df[] = {0, 1, 1, 1};
        const int dc[] = {1, 0, 1, -1};
        double valor = 0.0;

        for (int f = 0; f < filas; ++f) {
            for (int c = 0; c < columnas; ++c) {
                for (int d = 0; d < 4; ++d) {
                    int finF = f + (n - 1) * df[d];
                    int finC = c + (n - 1) * dc[d];
                    if (finF < 0 || finF >= filas || finC < 0 || finC >= columnas) continue;

                    int mias = 0;
                    int suyas = 0;
                    int libres = 0;

                    for (int k = 0; k < n; ++k) {
                        int celda = tablero.getCelda(f + k * df[d], c + k * dc[d]);
                        if (celda == id) ++mias;
                        else if (celda == rival) ++suyas;
                        else ++libres;
                    }

                    if (mias > 0 && suyas == 0) {
                        valor += pesoLinea(mias);
                        if (mias == n - 1 && libres == 1) valor += 250000.0;
                        else if (mias == n - 2 && libres >= 2) valor += 8000.0;
                    } else if (suyas > 0 && mias == 0) {
                        valor -= 1.25 * pesoLinea(suyas);
                        if (suyas == n - 1 && libres == 1) valor -= 320000.0;
                        else if (suyas == n - 2 && libres >= 2) valor -= 10000.0;
                    }
                }
            }
        }

        return valor;
    }
}

std::pair<int, int> SacarMovimiento(const Tablero& padre, const Tablero &hijo);

AgenteEstudiante::AgenteEstudiante(int id, int profundidadMax, double tiempoMax, int numHeuristica, ModoJuego modo) 
    : id(id), profundidadMax(profundidadMax), tiempoMaxSegundos(tiempoMax), numHeuristica(numHeuristica), modo(modo), abortarBanda(false) {
    nodosVisitados = 0;
}

bool AgenteEstudiante::tieneLimiteDeTiempo() const {
    return modo != ModoJuego::STATUS;
}

std::pair<int, int> AgenteEstudiante::think(const Tablero& tablero) {
    std::pair<int, int> mejor = {-1, -1};
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
        {
            auto sucesores = tablero.getSucesores();
            if (!sucesores.empty()) mejor = SacarMovimiento(tablero, sucesores.front());
        }
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
    int ganador = tablero.comprobarGanador();
    if (ganador == id) return Resultado::VICTORIA;
    if (ganador == rivalDe(id)) return Resultado::DERROTA;
    if (ganador == -1) return Resultado::EMPATE;

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return Resultado::EMPATE;

    bool maximizando = (tablero.getJugadorTurno() == id);
    Resultado mejorResultado = maximizando ? Resultado::DERROTA : Resultado::VICTORIA;
    Mov = SacarMovimiento(tablero, sucesores.front());

    for (const auto& hijo : sucesores) {
        std::pair<int,int> movHijo = {-1, -1};
        Resultado resultadoHijo = Status(hijo, movHijo);

        if ((maximizando && valorStatus(resultadoHijo) > valorStatus(mejorResultado)) ||
            (!maximizando && valorStatus(resultadoHijo) < valorStatus(mejorResultado))) {
            mejorResultado = resultadoHijo;
            Mov = SacarMovimiento(tablero, hijo);
        }

        if ((maximizando && mejorResultado == Resultado::VICTORIA) ||
            (!maximizando && mejorResultado == Resultado::DERROTA)) {
            break;
        }
    }

    return mejorResultado;
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
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, profundidad);
    if (profundidad >= prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    double mejorValor = maximizando ? MenosInfinito : MasInfinito;
    if (profundidad == 0) Mov = SacarMovimiento(tablero, sucesores.front());

    for (const auto& hijo : sucesores) {
        std::pair<int,int> movHijo = {-1, -1};
        double valor = minimax(hijo, profundidad + 1, prof_Max, movHijo);
        if (abortarBanda) {
            if (mejorValor == MenosInfinito || mejorValor == MasInfinito) return heuristica(tablero);
            return mejorValor;
        }

        if ((maximizando && valor > mejorValor) || (!maximizando && valor < mejorValor)) {
            mejorValor = valor;
            if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
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
    std::pair<int,int> Mov = {-1, -1};
    auto sucesores = tablero.getSucesores();
    if (!sucesores.empty()) Mov = SacarMovimiento(tablero, sucesores.front());

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
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, profundidad);
    if (profundidad >= prof_Max) return heuristica(tablero);

    auto sucesores = tablero.getSucesores();
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    std::stable_sort(sucesores.begin(), sucesores.end(),
        [&](const Tablero& a, const Tablero& b) {
            double va = heuristica(a);
            double vb = heuristica(b);
            return maximizando ? (va > vb) : (va < vb);
        });

    if (profundidad == 0) Mov = SacarMovimiento(tablero, sucesores.front());

    if (maximizando) {
        double mejorValor = MenosInfinito;
        for (const auto& hijo : sucesores) {
            std::pair<int,int> movHijo = {-1, -1};
            double valor = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
            if (abortarBanda) {
                if (mejorValor == MenosInfinito) return heuristica(tablero);
                return mejorValor;
            }

            if (valor > mejorValor) {
                mejorValor = valor;
                if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
            }

            alfa = std::max(alfa, mejorValor);
            if (alfa >= beta) break;
        }
        return mejorValor;
    }

    double mejorValor = MasInfinito;
    for (const auto& hijo : sucesores) {
        std::pair<int,int> movHijo = {-1, -1};
        double valor = alfaBeta(hijo, profundidad + 1, prof_Max, alfa, beta, movHijo);
        if (abortarBanda) {
            if (mejorValor == MasInfinito) return heuristica(tablero);
            return mejorValor;
        }

        if (valor < mejorValor) {
            mejorValor = valor;
            if (profundidad == 0) Mov = SacarMovimiento(tablero, hijo);
        }

        beta = std::min(beta, mejorValor);
        if (alfa >= beta) break;
    }

    return mejorValor;
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
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, 0);

    int rival = rivalDe(id);
    int filas = tablero.getFilas();
    int columnas = tablero.getColumnas();
    double valor = evaluarVentanas(tablero, id);

    double centroF = (filas - 1) / 2.0;
    double centroC = (columnas - 1) / 2.0;

    for (int f = 0; f < filas; ++f) {
        for (int c = 0; c < columnas; ++c) {
            int celda = tablero.getCelda(f, c);
            if (celda == 0) continue;

            double distanciaCentro = std::abs(f - centroF) + std::abs(c - centroC);
            double posicion = (filas + columnas) - distanciaCentro;

            if (celda == id) valor += 3.0 * posicion;
            else if (celda == rival) valor -= 3.2 * posicion;
        }
    }

    int n = tablero.getNParaGanar();
    if (n > 2) {
        valor += 80.0 * tablero.contarCombinaciones(n - 2, id);
        valor -= 95.0 * tablero.contarCombinaciones(n - 2, rival);
    }
    if (n > 1) {
        valor += 1200.0 * tablero.contarCombinaciones(n - 1, id);
        valor -= 1500.0 * tablero.contarCombinaciones(n - 1, rival);
    }

    return valor;
}

double AgenteEstudiante::heuristica2(const Tablero& tablero) {
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, 0);

    int rival = rivalDe(id);
    int n = tablero.getNParaGanar();
    double valor = heuristicaPrueba(tablero);

    valor += 0.35 * evaluarVentanas(tablero, id);
    if (n > 1) {
        valor += 350.0 * tablero.contarCombinaciones(n - 1, id);
        valor -= 420.0 * tablero.contarCombinaciones(n - 1, rival);
    }

    return valor;
}
