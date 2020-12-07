### Problema de múltiples productores y múltiples consumidores

#### int producir (int num_productor)

La función producir, ahora recibirá como parámetro el número de productor que la llama (un valor entre 0 y num_productores-1). De este dependerá el valor que se produzca:

- Cada productor producirá valores entre $num\_productor*c_p$ y $num\_productor*c_p+c_p-1$, siendo $c_p$ la cantidad de ítems que produce cada productor $c_p=\frac{num\_items}{num\_productores}$
- Por ello, el valor que devuelva la función producir en cada iteración será $c_p*num\_productor+contador$, comenzando $contador$ en 0 y llegando hasta $c_p-1$

#### void funcion_productor (int num_productor)

Ahora esta función también recibirá el mismo parámetro que indica el número del productor que la llama (entre 0 y num_productores-1). 

Cada productor producirá $\frac{num\_items}{num\_productores}$ elementos, según la función producir. 

Una vez producido el **valor**, un (**1**) entero (**MPI_INT**), se envía un mensaje de forma síncrona y segura, al buffer (**id_buffer**), con la etiqueta **etiq_productor** (valor 0), por el **MPI_COMM_WORLD**.

#### void consumir (int valor_cons)

Esta función no recibe modificaciones

#### void funcion_consumidor (int num_consumidor)

Ahora cada consumidor consumirá $c_c=\frac{num\_items}{num\_consumidores}$ ítems.

Para ello, al inicio de cada iteración, cada consumidor envía una petición al buffer (**MPI_Ssend**) y espera que este le responda extrayendo un valor del buffer (**MPI_Recv**). Una vez obtenido el valor del buffer, informa de ello por pantalla y llama a la función consumir (una espera aleatoria).

#### void funcion_buffer ()

Esta función se ve modificada:

- Ahora, según el estado del buffer, se asocia a la variable $tag$ el valor de una etiqueta. Si el buffer está vacío, sólo podrá insertar elementos, es decir, $tag=etiq\_productor$. Si está lleno, sólo puede extraer elementos, $tag=etiq\_consumidor$. En cambio, si se pueden hacer ambas cosas, $tag=MPI\_ANY\_TAG$, es decir, el buffer puede tanto insertar elementos como extraerlos.
- Una vez determinado se espera la petición del productor o el consumidor, en función del valor de $tag$ ( o de ambos).
- Si finalmente es un productor el que realiza la petición, simplemente inserta el valor en el buffer e informa de ello por pantalla.
- Si finalmente en cambio, es un consumidor el que realiza la petición, extrae el valor del buffer, informa de ello, y envía un mensaje (**MPI_Ssend**) de un (**1**) entero (**MPI_INT**) al destino que haya realizado la petición (**estado.MPI_SOURCE**) que tiene que tener la etiqueta del consumidor (**etiq_consumidor**) con el **MPI__COMM_WORLD**.

#### int main ()

En el main solo se realiza la introducción de la variable $n\_orden$, en la que identificamos, dentro de cada grupo (productores o consumidores) a cada consumidor.

- Para los productores, $n\_orden$ irá desde $0$ hasta $num\_productores-1$.

- Para los consumidores,$n\_orden$ irá desde $0$ hasta $num\_consumidores-1$.
- Como solo hay un buffer, este no necesita tener identificador $n\_orden$.

En cuanto a los identificadores de cada proceso, serán:

- **Productores**: Procesos con identificador entre $[0,num\_productores-1]$
- **Buffer**: Proceso con identificador $num\_productores$.
- **Consumidores**: Procesos con identificador entre $[num\_productores+1, num\_productores+num\_consumidores]$.



### Problema de los filosofos

### filosofos-interb

#### void funcion_filosofos (int id)

Función que ejecutarán los filósofos. Al principio se calculan los ids de los dos tenedores que necesitan y se hace un bucle infinito en el que:

- Se envía un mensaje con la etiqueta <etiq_coger> al tenedor de la izquierda. Es decir se solicita acceso al tenedor de su izquierda.
- Se envía un mensaje con la etiqueta <etiq_coger> al tenedor de la derecha. Es decir se solicita acceso a él.
- Se hace una espera aleatoria (el tiempo que está comiendo)
- Se suelta el tenedor izquierdo, enviando un mensaje al mismo id_ten_izq para que pueda ser asignado a otro proceso filósofo. Esto se hace enviando un mensaje con la etiqueta <etiq_soltar>
- Se hace lo propio con el tenedor derecho.

#### void funcion_tenedores (int id)

En un bucle infinito:

- Se espera que un filósofo coja el tenedor (MPI_Recv(...)), es decir, espera un mensaje con la etiqueta <etiq_coger>
- Se informa de ello
- Espera que el mismo filósofo suelte el tenedor (MPI_Recv(...)), espera otro mensaje ahora con la etiqueta <etiq_soltar>

#### int main () 

Para cada proceso:

- Si es un proceso par, ejecutará la función de los filósofos
- Si es impar, será un tenedor

Se comprueba siempre que se haya llamado al programa con el número correcto de procesos.

#### El interbloqueo

Esta solución puede conducir a error, pues se puede dar el caso en el que **todos los filósofos quieran coger el tenedor de su izquierda, antes de que ninguno pueda acceder a coger el de su derecha**. En este caso se produciría una situación de interbloqueo, pues todos los procesos filósofos estarían bloqueando los procesos tenedor de su izquierda, y ninguno podría acceder al de su derecha porque a su vez está ocupado por el filósofo anterior.



### La solución

Para solucionar este problema haremos simplemente que un filósofo (el 0 por simplicidad), acceda primero al tenedor de su derecha y después al de su izquierda. De esta forma, suponiendo que el proceso 0 (filósofo 0), se quede bloqueado porque el proceso 8 (filósofo 4) está bloqueando el tenedor 4 (proceso 9) y el propio filósofo 4 siga bloqueado, etc. hasta llegar al filósofo 1, el filósofo 1 sí que podrá desbloquearse, porque puede acceder al tenedor de su izquierda (porque el filósofo 2 está bloqueado intentando coger el tenedor de su izquierda), y a la vez también puede acceder al de su derecha (tenedor 1), porque el filósofo 0 no está tratando de coger el tenedor 1, porque está bloqueado tratando de coger el tenedor 5, es decir, el de su derecha.

