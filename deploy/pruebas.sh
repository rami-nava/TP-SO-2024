#!/bin/bash                                                                                                                                                                                                        

echo "Seleccione la prueba a realizar:"
echo "1)Base"
echo "2)Recursos"
echo "3)Memoria"
echo "4)IO"
echo "5)FS"
echo "6)Salvation's Edge"

read -p "Numero de prueba: " prueba

case $prueba in
    1)
        echo "Prueba Base"
        cp "../deploy/Base/memoria_base.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Base/cpu_base.config" "../cpu/cfg/cpu.config"
        cp "../deploy/Base/SLP1.config" "../entradasalida/cfg/SLP1.config"

        echo "1. FIFO"
        echo "2. RR"
        echo "3. VRR"
        read -p "Algoritmo a usar:" algoritmo
        case $algoritmo in
            1)
                cp "../deploy/Base/kernel_baseFIFO.config" "../kernel/cfg/kernel.config"
                ;;
            2)
                cp "../deploy/Base/kernel_baseRR.config" "../kernel/cfg/kernel.config"
                ;;
            3)
                cp "../deploy/Base/kernel_baseVRR.config" "../kernel/cfg/kernel.config"
                ;;
        esac
        echo "Configuraciones seteadas correctamente para Prueba Base"
        ;;
    2)
        echo "Prueba Recursos"
        cp "../deploy/Recursos/memoria_recursos.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Recursos/ESPERA.config" "../entradasalida/cfg/ESPERA.config"
        cp "../deploy/Recursos/kernel_recursos.config" "../kernel/cfg/kernel.config"
        cp "../deploy/Recursos/cpu_recursos.config" "../cpu/cfg/cpu.config"
        echo "Configuraciones seteadas correctamente para Prueba Recursos"
        ;;
    3)
        echo "Prueba Memoria"
        cp "../deploy/Memoria/kernel_memoria.config" "../kernel/cfg/kernel.config"
        cp "../deploy/Memoria/memoria_memoria.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Memoria/IO_GEN_SLEEP.config" "../entradasalida/cfg/IO_GEN_SLEEP.config"

        echo "1. FIFO"
        echo "2. LRU"
        read -p "Algoritmo a usar en TLB: " algMem
        case $algMem in
            1)
                cp "../deploy/Memoria/cpu_memoriaFIFO.config" "../cpu/cfg/cpu.config"
                ;;
            2)
                cp "../deploy/Memoria/cpu_memoriaLRU.config" "../cpu/cfg/cpu.config"
                ;;
        esac
        echo "Configuraciones seteadas correctamente para Prueba memoria"
        ;;

    4)
        echo "Prueba IO"
        cp "../deploy/IO/kernel_io.config" "../kernel/cfg/kernel.config"
        cp "../deploy/IO/cpu_io.config" "../cpu/cfg/cpu.config"
        cp "../deploy/IO/memoria_io.config" "../memoria/cfg/memoria.config"
        cp "../deploy/IO/GENERICA.config" "../entradasalida/cfg/GENERICA.config"
        cp "../deploy/IO/TECLADO.config" "../entradasalida/cfg/TECLADO.config"
        cp "../deploy/IO/MONITOR.config" "../entradasalida/cfg/MONITOR.config"
        echo "Configuraciones seteadas correctamente para Prueba IO"
        ;;
    5)
        echo "Prueba FS"
        cp "../deploy/FS/kernel_fs.config" "../kernel/cfg/kernel.config"
        cp "../deploy/FS/cpu_fs.config" "../cpu/cfg/cpu.config"
        cp "../deploy/FS/memoria_fs.config" "../memoria/cfg/memoria.config"
        cp "../deploy/FS/FILESYSTEM.config" "../entradasalida/cfg/FILESYSTEM.config"
        cp "../deploy/FS/TECLADO.config" "../entradasalida/cfg/TECLADO.config"
        cp "../deploy/FS/MONITOR.config" "../entradasalida/cfg/MONITOR.config"
        echo "Configuraciones seteadas correctamente para Prueba FS"
        ;;
    6)
        echo "Prueba Salvation's Edge"
        cp "../deploy/SE/kernel_se.config" "../kernel/cfg/kernel.config"
        cp "../deploy/SE/cpu_se.config" "../cpu/cfg/cpu.config"
        cp "../deploy/SE/memoria_se.config" "../memoria/cfg/memoria.config"
        cp "../deploy/SE/GENERICA.config" "../entradasalida/cfg/GENERICA.config"
        cp "../deploy/SE/TECLADO.config" "../entradasalida/cfg/TECLADO.config"
        cp "../deploy/SE/MONITOR.config" "../entradasalida/cfg/MONITOR.config"
        cp "../deploy/SE/ESPERA.config" "../entradasalida/cfg/ESPERA.config"
        cp "../deploy/SE/SLP1.config" "../entradasalida/cfg/SLP1.config"
        echo "Configuraciones seteadas correctamente para Prueba Salvation's Edge"
        ;;
    *)
        echo "Comando no reconocido"
        ;;
esac

exit 0





