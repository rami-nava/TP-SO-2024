#!/bin/bash
validate_ip() {
  # Regular expression pattern for IP address validation
  local ip_pattern='^([0-9]{1,3}.){3}[0-9]{1,3}$'

  if [[ $1 =~ $ip_pattern ]]; then
    return 0  # Valid IP address
  else
    return 1  # Invalid IP address
  fi
}

# Take the replace string
read -p "Ingrese kernel IP: " ipkernel
read -p "Ingrese Memoria IP: " ipMemoria
read -p "Ingrese CPU IP: " ipCPU

# Regexs
sKer="IP_KERNEL=[0-9\.]*"
sMem="IP_MEMORIA=[0-9\.]*"
sCpu="IP_CPU=[0-9\.]*"

# kernel
kc0="../kernel/cfg/kernel.config"
kc1="../deploy/Base/kernel_baseFIFO.config"
kc2="../deploy/Base/kernel_baseRR.config"
kc3="../deploy/Base/kernel_baseVRR.config"
kc4="../deploy/Recursos/kernel_recursos.config"
kc5="../deploy/Memoria/kernel_memoria.config"
kc6="../deploy/IO/kernel_io.config"
kc7="../deploy/SE/kernel_se.config"
kc8="../deploy/FS/kernel_fs.config"

# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc6
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc7
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc8

# Search and replace CPU
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc0
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc1
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc2
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc3
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc4
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc5
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc6
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc7
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc8

# entradasalida
entradasalida1="../deploy/Base/SLP1.config"
entradasalida2="../deploy/Recursos/ESPERA.config"
entradasalida3="../deploy/Memoria/IO_GEN_SLEEP.config"
entradasalida4="../deploy/FS/FILESYSTEM.config"
entradasalida5="../deploy/FS/MONITOR.config"
entradasalida6="../deploy/IO/GENERICA.config"
entradasalida7="../deploy/IO/MONITOR.config"
entradasalida8="../deploy/IO/TECLADO.config"
entradasalida9="../deploy/FS/TECLADO.config"
entradasalida10="../deploy/SE/TECLADO.config"
entradasalida11="../deploy/SE/MONITOR.config"
entradasalida12="../deploy/SE/SLP1.config"
entradasalida13="../deploy/SE/GENERICA.config"
entradasalida14="../deploy/SE/ESPERA.config"


# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida6
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida7
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida8
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida9
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida10
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida11
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida12
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida13
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $entradasalida14

# Search and replace KERNEL
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida1
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida2
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida3
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida4
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida5
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida6
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida7
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida8
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida9
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida10
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida11
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida12
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida13
sed -E -i "s/$sMem/IP_KERNEL=$ipkernel/" $entradasalida14

# cpu
cpc0="../cpu/cfg/cpu.config"
cpc1="../deploy/Base/cpu_base.config"
cpc2="../deploy/Recursos/cpu_recursos.config"
cpc3="../deploy/Memoria/cpu_memoriaLRU.config"
cpc4="../deploy/Memoria/cpu_memoriaFIFO.config"
cpc5="../deploy/IO/cpu_io.config"
cpc6="../deploy/FS/cpu_fs.config"
cpc7="../deploy/SE/cpu_se.config"

# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc6
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc7

# memoria
mc0="../memoria/cfg/memoria.config"
mc1="../deploy/Base/memoria_base.config"
mc2="../deploy/Recursos/memoria_recursos.config"
mc3="../deploy/Memoria/memoria_memoria.config"
mc4="../deploy/FS/memoria_fs.config"
mc5="../deploy/SE/memoria_se.config"
mc6="../deploy/IO/memoria_io.config"

# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc6



echo "Las IPs han sido  modificadas correctamente"
