# Trabajo Práctico - Sistemas Operativos - 2018 1C
### TP-2018-1C-GRUPO

* [dantebado](https://github.com/dantebado) - Dante Bado
* [theMec](https://github.com/theMec) - Martín Calviello
* [Magli44](https://github.com/Magli44) - Matías Iglesias
* [npyt](https://github.com/npyt) - Ramiro Maranzana
* [rangolucas](https://github.com/rangolucas) - Lucas Rango

## Links

* [Enunciado](https://sisoputnfrba.gitbook.io/re-distinto/) - Re Distinto (2018-1c) GitBook
* [Foro](https://github.com/sisoputnfrba/foro/issues) - Issues

## CheckPoints

### [Checkpoint 1 - No presencial](https://github.com/sisoputnfrba/2018-1c-re-distinto/blob/master/descripcion-entregas.md#checkpoint-1---no-presencial)

- [X] Creación de todos los procesos que intervienen
- [X] Desarrollar una comunicación simple entre los procesos que permita propagar un mensaje por cada conexión necesaria
- [X] Implementar la consola del Planificador sin funcionalidades

### [Checkpoint 2 - No presencial](https://sisoputnfrba.gitbook.io/re-distinto/descripcion-de-las-entregas#checkpoint-2-no-presencial)

- [ ] Lectura de scripts y utilización del Parser del proceso ESI
- [ ] El Planificador debe ser capaz de elegir a un ESI utilizando un algoritmo sencillo (FIFO por ej)
- [X] El Coordinador debe ser capaz de distribuir por Equitative Load.
- [X] Desarrollo de lectura y escritura de Entradas en el Instancia (Operaciones GET/SET).

### [Checkpoint 3 - Presencial - Laboratorio](https://sisoputnfrba.gitbook.io/re-distinto/descripcion-de-las-entregas#checkpoint-3-presencial-laboratorio)

- [ ] ESI completo
- [ ] Planificador utilizando SJF con y sin desalojo, con todas sus colas.
- [ ] La consola del Planificador deberá poder ejecutar los comandos "Pausar/Continuar", "Bloquear", "Desbloquear" y "Listar"
- [ ] El Coordinador deberá tener el "Log de Operaciones" funcionando. También deberá ser capaz de comunicar bloqueos.
- [ ] La Instancia deberá implementar todas las instrucciones. A la hora de reemplazar claves, deberá implementar el algoritmo Circular

### [Checkpoint 4 - No Presencial](https://sisoputnfrba.gitbook.io/re-distinto/descripcion-de-las-entregas#checkpoint-4-no-presencial)

- [ ] Planificador utilizando HRRN
- [ ] La consola del Planificador deberá poder ejecutar los comandos "kill" y "status"
- [ ] El Coordinador deberá ser capaz de distribuir utilizando "LSU" y "KE". Implementar retardos
- [ ] La Instancia deberá ser capaz de soportar desconexiones y reincorporaciones. Además se deberá implementar el algoritmo LRU

### [Entrega final - Presencial - Laboratorio](https://sisoputnfrba.gitbook.io/re-distinto/descripcion-de-las-entregas#entrega-final-presencial-laboratorio)

- [ ]  La consola del Planificador deberá poder ejecutar el comando "deadlock"
- [ ]  La Instancia deberá ser capaz de soportar dumps y compactación. Se deberá implementar el algoritmo BSU
