# Problema Productor Consumidor
El objetivo del programa es implementar un sistema productor-consumidor para gestionar
pedidos de comida. El programa utiliza hilos (threads), memoria compartida y semáforos para la
sincronización entre los hilos productor y consumidor. El productor lee pedidos de un archivo de
pedidos y los coloca en una cola en memoria compartida. El consumidor lee los pedidos de la
cola, los consume (eliminando la línea correspondiente en el archivo de pedidos) y registra el
consumo en un archivo de registros. El programa permite que varios productores y
consumidores trabajen de manera concurrente.
