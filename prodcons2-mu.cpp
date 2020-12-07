// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

// Los identificadores:
// 0-(num_productores-1)                                  = productores
// num_productores                                        = buffer
// (num_productores+1)-(num_productores+num_consumidores) = consumidores

const int
    num_items = 20,
    tam_vector = 10,
    num_productores = 4,
    num_consumidores = 5,
    etiq_productor = 0,
    etiq_consumidor = 1,
    num_procesos_esperado = num_productores + num_consumidores + 1,
    id_buffer = num_productores;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template <int min, int max>
int aleatorio()
{
   static default_random_engine generador((random_device())());
   static uniform_int_distribution<int> distribucion_uniforme(min, max);
   return distribucion_uniforme(generador);
}
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int num_productor)
{
   static int contador = 0;
   sleep_for(milliseconds(aleatorio<10, 100>()));
   int producido = (num_productor*(num_items/num_productores)+contador);
   cout << "Productor " << num_productor << " ha producido valor " << producido << endl
        << flush;
   contador++;

   return producido;
}
// ---------------------------------------------------------------------
// num_productor va de 0-(num_productor+1)
void funcion_productor(int num_productor)
{
   for (unsigned int i = 0; i < num_items / num_productores; i++)
   {
      // producir valor
      int valor_prod = producir(num_productor);
      // enviar valor
      cout << "Productor " << num_productor << " va a enviar valor " << valor_prod << endl
           << flush;
      MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, etiq_productor, MPI_COMM_WORLD);
   }
}
// ---------------------------------------------------------------------

void consumir(int valor_cons)
{
   // espera bloqueada
   sleep_for(milliseconds(aleatorio<110, 200>()));
   cout << "Consumidor ha consumido valor " << valor_cons << endl
        << flush;
}
// ---------------------------------------------------------------------
// num_consumidor va de 0-(num_consumidor-1)
void funcion_consumidor(int num_consumidor)
{
   int peticion,
       valor_rec = 1;
   MPI_Status estado;

   for (int i = 0; i < num_items / num_consumidores; i++)
   {
      MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
      MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD, &estado);
      cout << "Consumidor " << num_consumidor << " ha recibido valor " << valor_rec << endl << flush;
      consumir(valor_rec);
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int buffer[tam_vector],      // buffer con celdas ocupadas y vacías
       valor,                   // valor recibido o enviado
       primera_libre = 0,       // índice de primera celda libre
       primera_ocupada = 0,     // índice de primera celda ocupada
       num_celdas_ocupadas = 0, // número de celdas ocupadas
       tag;
   MPI_Status estado; // metadatos del mensaje recibido

   for (unsigned int i = 0; i < num_items * 2; i++)
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if (num_celdas_ocupadas == 0)
         tag = etiq_productor;
      else if (num_celdas_ocupadas == tam_vector)
         tag = etiq_consumidor;
      else
         tag = MPI_ANY_TAG;
      
      // 2. Recibir mensaje de    cualquier fuente con la etiqueta tag
      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &estado);

      // 3. Procesar el mensaje recibido
      // Si ha recibido un valor producido
      if (estado.MPI_TAG == etiq_productor)
      {
         buffer[primera_libre] = valor;
         primera_libre = (primera_libre + 1) % tam_vector;
         num_celdas_ocupadas++;
         cout << "Buffer ha recibido valor " << valor << endl << flush;
      }
      else if (estado.MPI_TAG == etiq_consumidor)
      {
         valor = buffer[primera_ocupada];
         primera_ocupada = (primera_ocupada + 1) % tam_vector;
         num_celdas_ocupadas--;
         cout << "Buffer va a enviar valor " << valor << endl << flush;
         MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_consumidor, MPI_COMM_WORLD);
      }
   }
}

// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
   int id_propio, num_procesos_actual, n_orden;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

   if (id_propio < num_productores)
      n_orden = id_propio;
   if (id_propio > num_productores)
      n_orden = id_propio % (num_productores + 1);

   if (num_procesos_esperado == num_procesos_actual)
   {
      // ejecutar la operación apropiada a 'id_propio'
      if (id_propio < num_productores)
         funcion_productor(n_orden);
      else if (id_propio == num_productores)
         funcion_buffer();
      else if (id_propio > num_productores)
         funcion_consumidor(n_orden);
   }
   else
   {
      if (id_propio == 0) // solo el primero escribe error, indep. del rol
      {
         cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
              << "el número de procesos en ejecución es: " << num_procesos_actual << endl
              << "(programa abortado)" << endl;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize();
   return 0;
}
