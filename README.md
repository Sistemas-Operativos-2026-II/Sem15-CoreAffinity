# CPU Scheduling y Core Affinity — Laboratorio Linux

**Sistemas Operativos CS3015 · 2026-2**

> Ejecutar desde la raíz del repositorio. No se requieren privilegios de administrador. Los bloques son comandos independientes, no un script único.

## 0. Preparación

```bash
uname -s
uname -r
for cmd in gcc lscpu nproc ps top taskset nice renice; do
    command -v "$cmd" >/dev/null && echo "[OK] $cmd" || echo "[FALTA] $cmd"
done

gcc -O2 -std=c11 -Wall -Wextra -o cpu-worker src/cpu-worker.c
gcc -O2 -std=c11 -Wall -Wextra -o cpu-info src/cpu-info.c
./cpu-worker --seconds 2
```

## 1. Topología y CPU disponibles

```bash
lscpu
nproc
lscpu -e=CPU,CORE,SOCKET,ONLINE
taskset -pc $$
./cpu-info
```

Asignar **dos identificadores permitidos** según la salida anterior; `0` y `1` son solo ejemplos:

```bash
CPU_A=0
CPU_B=1
```

## 2. Observación de procesos

```bash
./cpu-worker --seconds 12 &
pid1=$!
./cpu-worker --seconds 12 &
pid2=$!
printf 'PID1=%s PID2=%s\n' "$pid1" "$pid2"
./cpu-info "$pid1"
./cpu-info "$pid2"
ps -o pid,psr,pcpu,ni,stat,comm -p "$pid1","$pid2"
top -p "$pid1","$pid2"   # Salir con q
wait "$pid1" "$pid2"
```

## 3. CPU Affinity — `taskset`

### 3.1. Misma CPU lógica

```bash
taskset -c "$CPU_A" ./cpu-worker --iterations 300000000 &
pid1=$!
taskset -c "$CPU_A" ./cpu-worker --iterations 300000000 &
pid2=$!
wait "$pid1" "$pid2"
```

### 3.2. CPU lógicas diferentes

```bash
taskset -c "$CPU_A" ./cpu-worker --iterations 300000000 &
pid1=$!
taskset -c "$CPU_B" ./cpu-worker --iterations 300000000 &
pid2=$!
wait "$pid1" "$pid2"
```

Comparar `tiempo_real` y `tiempo_cpu`. Para equipos más lentos, reducir **el mismo** número de iteraciones en ambas pruebas.

## 4. Prioridad — `nice`

```bash
taskset -c "$CPU_A" nice -n 0 ./cpu-worker --seconds 10 &
pid1=$!
taskset -c "$CPU_A" nice -n 15 ./cpu-worker --seconds 10 &
pid2=$!
ps -o pid,psr,ni,pcpu,comm -p "$pid1","$pid2"
wait "$pid1" "$pid2"
```

Comparar `tiempo_cpu` e `iteraciones`; ambas ejecuciones usan una ventana temporal similar.

## 5. Inspección opcional de un proceso propio

```bash
./cpu-worker --seconds 15 &
pid=$!
ps -o pid,psr,ni,pcpu,stat,comm -p "$pid"
taskset -pc "$pid"
./cpu-info "$pid"
renice -n 10 -p "$pid"
ps -o pid,psr,ni,pcpu,stat,comm -p "$pid"
wait "$pid"
```

**Seguridad:** no ejecutar como `root`; no modificar procesos ajenos ni políticas de tiempo real. Los procesos terminan automáticamente. Si se requiere detener cargas en segundo plano, ejecutar `kill "$pid1" "$pid2"` únicamente con PID verificados y correspondientes a esta práctica.
