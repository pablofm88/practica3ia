#include "AgenteEstudiante.hpp"
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>
#include <cstdlib>

std::pair<int, int> SacarMovimiento(const Tablero& padre, const Tablero &hijo);

namespace {
    struct SucesorConMovimiento {
        Tablero tablero;
        std::pair<int, int> movimiento;
        double valorOrden;
    };

    int rivalDe(int jugador) {
        return (jugador == 1) ? 2 : 1;
    }

    int valorStatus(AgenteEstudiante::Resultado resultado) {
        if (resultado == AgenteEstudiante::Resultado::VICTORIA) return 1;
        if (resultado == AgenteEstudiante::Resultado::EMPATE) return 0;
        return -1;
    }

    double valorTerminal(const Tablero& tablero, int id, int profundidad) {
        int ganador = tablero.comprobarGanador();
        if (ganador == id) return AgenteEstudiante::GANAR - profundidad;
        if (ganador == rivalDe(id)) return AgenteEstudiante::PERDER + profundidad;
        return 0.0;
    }

    std::vector<SucesorConMovimiento> obtenerSucesores(const Tablero& tablero) {
        std::vector<SucesorConMovimiento> resultado;
        auto sucesores = tablero.getSucesores();
        resultado.reserve(sucesores.size());

        for (const auto& hijo : sucesores) {
            resultado.push_back({hijo, SacarMovimiento(tablero, hijo), 0.0});
        }

        return resultado;
    }

    bool esMovimientoValido(const Tablero& tablero, std::pair<int, int> mov) {
        if (mov.first == -1 && mov.second == -1) {
            return !tablero.tieneMovimientosValidos();
        }

        Tablero copia = tablero;
        return copia.ponerPieza(mov.first, mov.second, tablero.getJugadorTurno());
    }

    std::pair<int, int> primerMovimientoValido(const Tablero& tablero) {
        auto sucesores = obtenerSucesores(tablero);
        if (!sucesores.empty()) return sucesores.front().movimiento;
        return {-1, -1};
    }

    bool esModoCompeticion(const Tablero& tablero) {
        return tablero.getFilas() == 9 && tablero.getColumnas() == 9 && tablero.getNParaGanar() == 5;
    }

    bool movimientoLegalSinSimular(const Tablero& tablero, int f, int c) {
        if (f < 0 || f >= tablero.getFilas() || c < 0 || c >= tablero.getColumnas()) return false;
        if (tablero.getCelda(f, c) != 0) return false;

        if (!esModoCompeticion(tablero)) return true;
        if ((f + c) % 3 != tablero.getFaseActual() % 3) return false;
        return tablero.esVacio() || tablero.tieneAdyacente(f, c);
    }

    double pesoLinea(int piezas, int n) {
        if (piezas <= 0) return 0.0;
        if (piezas >= n) return 500000000.0;

        static const double pesosBase[] = {
            0.0, 8.0, 75.0, 1400.0, 70000.0, 3000000.0,
            12000000.0, 50000000.0, 200000000.0, 500000000.0
        };
        int numPesos = static_cast<int>(sizeof(pesosBase) / sizeof(pesosBase[0]));
        if (piezas < numPesos) return pesosBase[piezas];
        return pesosBase[numPesos - 1];
    }

    double factorHuecosEspeciales(int rojos, int amarillos, int verdes, int direccion) {
        double factor = 1.0;

        if (rojos > 0) factor *= 0.18;
        if (verdes > 0) factor *= 1.10;

        // Las bombas rompen con especial dureza las amenazas horizontales y verticales.
        if (amarillos > 0) {
            if (direccion == 0 || direccion == 1) factor *= 0.30;
            else factor *= 0.75;
        }

        return factor;
    }

    double evaluarVentanas(const Tablero& tablero, int id, bool avanzada) {
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
                    int rojos = 0;
                    int amarillos = 0;
                    int verdes = 0;

                    for (int k = 0; k < n; ++k) {
                        int nf = f + k * df[d];
                        int nc = c + k * dc[d];
                        int celda = tablero.getCelda(nf, nc);

                        if (celda == id) {
                            ++mias;
                        } else if (celda == rival) {
                            ++suyas;
                        } else {
                            ++libres;
                            auto tipo = tablero.getTipoCelda(nf, nc);
                            if (tipo == Tablero::TipoCelda::ROJO) ++rojos;
                            else if (tipo == Tablero::TipoCelda::AMARILLO) ++amarillos;
                            else if (tipo == Tablero::TipoCelda::VERDE) ++verdes;
                        }
                    }

                    double factorEspecial = avanzada ? factorHuecosEspeciales(rojos, amarillos, verdes, d) : 1.0;

                    if (mias > 0 && suyas == 0) {
                        double puntos = pesoLinea(mias, n) * factorEspecial;
                        if (mias == n - 1 && libres == 1) puntos += 1200000.0 * factorEspecial;
                        else if (mias == n - 2 && libres >= 2) puntos += 45000.0 * factorEspecial;
                        valor += puntos;
                    } else if (suyas > 0 && mias == 0) {
                        double puntos = pesoLinea(suyas, n) * factorEspecial;
                        if (suyas == n - 1 && libres == 1) puntos += 1500000.0 * factorEspecial;
                        else if (suyas == n - 2 && libres >= 2) puntos += 55000.0 * factorEspecial;
                        valor -= (avanzada ? 1.28 : 1.12) * puntos;
                    }
                }
            }
        }

        return valor;
    }

    double evaluarControlPosicional(const Tablero& tablero, int id) {
        int rival = rivalDe(id);
        int filas = tablero.getFilas();
        int columnas = tablero.getColumnas();
        double centroF = (filas - 1) / 2.0;
        double centroC = (columnas - 1) / 2.0;
        double valor = 0.0;

        for (int f = 0; f < filas; ++f) {
            for (int c = 0; c < columnas; ++c) {
                int celda = tablero.getCelda(f, c);
                if (celda == 0) continue;

                double distancia = std::abs(f - centroF) + std::abs(c - centroC);
                double posicion = (filas + columnas) - 1.7 * distancia;
                if (celda == id) valor += 9.0 * posicion;
                else if (celda == rival) valor -= 9.5 * posicion;
            }
        }

        return valor;
    }

    double evaluarOpcionesEspeciales(const Tablero& tablero, int id) {
        if (!esModoCompeticion(tablero)) return 0.0;

        int jugador = tablero.getJugadorTurno();
        double signo = (jugador == id) ? 1.0 : -1.0;
        double valor = 0.0;
        int legales = 0;

        for (int f = 0; f < tablero.getFilas(); ++f) {
            for (int c = 0; c < tablero.getColumnas(); ++c) {
                if (!movimientoLegalSinSimular(tablero, f, c)) continue;
                ++legales;

                auto tipo = tablero.getTipoCelda(f, c);
                if (tipo == Tablero::TipoCelda::VERDE) {
                    valor += signo * 260.0;
                } else if (tipo == Tablero::TipoCelda::ROJO) {
                    valor -= signo * 650.0;
                } else if (tipo == Tablero::TipoCelda::AMARILLO) {
                    int propias = 0;
                    int rivales = 0;
                    int rival = rivalDe(jugador);
                    for (int i = 0; i < tablero.getFilas(); ++i) {
                        if (i == f) continue;
                        if (tablero.getCelda(i, c) == jugador) ++propias;
                        else if (tablero.getCelda(i, c) == rival) ++rivales;
                    }
                    for (int j = 0; j < tablero.getColumnas(); ++j) {
                        if (j == c) continue;
                        if (tablero.getCelda(f, j) == jugador) ++propias;
                        else if (tablero.getCelda(f, j) == rival) ++rivales;
                    }
                    valor += signo * 90.0 * (rivales - propias);
                }
            }
        }

        valor += signo * 4.0 * legales;
        valor += (tablero.getJugadorTurno() == id ? 1.0 : -1.0) * 35.0 * tablero.getMovimientosRestantes();
        return valor;
    }
}

AgenteEstudiante::AgenteEstudiante(int id, int profundidadMax, double tiempoMax, int numHeuristica, ModoJuego modo) 
    : id(id), profundidadMax(profundidadMax), tiempoMaxSegundos(tiempoMax), numHeuristica(numHeuristica), modo(modo), abortarBanda(false) {
    nodosVisitados = 0;
}

bool AgenteEstudiante::tieneLimiteDeTiempo() const {
    return modo != ModoJuego::STATUS;
}

std::pair<int, int> AgenteEstudiante::think(const Tablero& tablero) {
    std::pair<int, int> mejor = primerMovimientoValido(tablero);
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
        if (!esMovimientoValido(tablero, mejor)) mejor = primerMovimientoValido(tablero);
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

    auto sucesores = obtenerSucesores(tablero);
    if (sucesores.empty()) return Resultado::EMPATE;

    bool maximizando = (tablero.getJugadorTurno() == id);
    Resultado mejorResultado = maximizando ? Resultado::DERROTA : Resultado::VICTORIA;
    Mov = sucesores.front().movimiento;

    for (const auto& sucesor : sucesores) {
        std::pair<int,int> movHijo = {-1, -1};
        Resultado resultadoHijo = Status(sucesor.tablero, movHijo);

        bool mejora = maximizando
            ? valorStatus(resultadoHijo) > valorStatus(mejorResultado)
            : valorStatus(resultadoHijo) < valorStatus(mejorResultado);

        if (mejora) {
            mejorResultado = resultadoHijo;
            Mov = sucesor.movimiento;
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
    
    if (tiempoMaxSegundos > 0.0 &&
        std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, profundidad);
    if (profundidad >= prof_Max) return heuristica(tablero);

    auto sucesores = obtenerSucesores(tablero);
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    double mejorValor = maximizando ? MenosInfinito : MasInfinito;
    if (profundidad == 0) Mov = sucesores.front().movimiento;

    for (const auto& sucesor : sucesores) {
        std::pair<int,int> movHijo = {-1, -1};
        double valor = minimax(sucesor.tablero, profundidad + 1, prof_Max, movHijo);

        if (abortarBanda) {
            if (mejorValor == MenosInfinito || mejorValor == MasInfinito) return heuristica(tablero);
            return mejorValor;
        }

        bool mejora = maximizando ? valor > mejorValor : valor < mejorValor;
        if (mejora) {
            mejorValor = valor;
            if (profundidad == 0) Mov = sucesor.movimiento;
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
    std::pair<int,int> Mov = primerMovimientoValido(tablero);
    mejorMovimientoH = Mov;
    double valor = heuristica(tablero);

    for (int profundidad = 1; profundidad <= std::max(1, profundidadMax); ++profundidad) {
        std::pair<int,int> movProfundidad = mejorMovimientoH;
        double valorProfundidad = alfaBeta(tablero, 0, profundidad, MenosInfinito, MasInfinito, movProfundidad);

        if (abortarBanda) break;
        if (esMovimientoValido(tablero, movProfundidad)) {
            valor = valorProfundidad;
            Mov = movProfundidad;
            mejorMovimientoH = Mov;
        }
    }

    if (!esMovimientoValido(tablero, Mov)) Mov = primerMovimientoValido(tablero);
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
    
    if (tiempoMaxSegundos > 0.0 &&
        std::chrono::duration<double>(std::chrono::steady_clock::now() - inicioBusqueda).count() > tiempoMaxSegundos) {
        abortarBanda = true;
        return 0;
    }
    /* ============== Empieza a partir de aquí tu implementación  =============== */
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, profundidad);
    if (profundidad >= prof_Max) return heuristica(tablero);

    auto sucesores = obtenerSucesores(tablero);
    if (sucesores.empty()) return heuristica(tablero);

    bool maximizando = (tablero.getJugadorTurno() == id);
    for (auto& sucesor : sucesores) {
        sucesor.valorOrden = heuristica(sucesor.tablero);
    }

    std::stable_sort(sucesores.begin(), sucesores.end(),
        [maximizando](const SucesorConMovimiento& a, const SucesorConMovimiento& b) {
            return maximizando ? a.valorOrden > b.valorOrden : a.valorOrden < b.valorOrden;
        });

    if (profundidad == 0) Mov = sucesores.front().movimiento;

    if (maximizando) {
        double mejorValor = MenosInfinito;

        for (const auto& sucesor : sucesores) {
            std::pair<int,int> movHijo = {-1, -1};
            double valor = alfaBeta(sucesor.tablero, profundidad + 1, prof_Max, alfa, beta, movHijo);

            if (abortarBanda) {
                if (mejorValor == MenosInfinito) return heuristica(tablero);
                return mejorValor;
            }

            if (valor > mejorValor) {
                mejorValor = valor;
                if (profundidad == 0) Mov = sucesor.movimiento;
            }

            alfa = std::max(alfa, mejorValor);
            if (alfa >= beta) break;
        }

        return mejorValor;
    }

    double mejorValor = MasInfinito;

    for (const auto& sucesor : sucesores) {
        std::pair<int,int> movHijo = {-1, -1};
        double valor = alfaBeta(sucesor.tablero, profundidad + 1, prof_Max, alfa, beta, movHijo);

        if (abortarBanda) {
            if (mejorValor == MasInfinito) return heuristica(tablero);
            return mejorValor;
        }

        if (valor < mejorValor) {
            mejorValor = valor;
            if (profundidad == 0) Mov = sucesor.movimiento;
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
    int n = tablero.getNParaGanar();
    double valor = 0.0;

    valor += evaluarVentanas(tablero, id, true);
    valor += evaluarControlPosicional(tablero, id);
    valor += evaluarOpcionesEspeciales(tablero, id);

    if (n > 2) {
        valor += 220.0 * tablero.contarCombinaciones(n - 2, id);
        valor -= 270.0 * tablero.contarCombinaciones(n - 2, rival);
    }
    if (n > 1) {
        valor += 6000.0 * tablero.contarCombinaciones(n - 1, id);
        valor -= 7600.0 * tablero.contarCombinaciones(n - 1, rival);
    }

    return valor;
}

double AgenteEstudiante::heuristica2(const Tablero& tablero) {
    int ganador = tablero.comprobarGanador();
    if (ganador != 0) return valorTerminal(tablero, id, 0);

    int rival = rivalDe(id);
    int n = tablero.getNParaGanar();
    double valor = heuristicaPrueba(tablero);

    valor += 0.45 * evaluarVentanas(tablero, id, false);
    if (n > 1) {
        valor += 1800.0 * tablero.contarCombinaciones(n - 1, id);
        valor -= 2200.0 * tablero.contarCombinaciones(n - 1, rival);
    }

    return valor;
}
