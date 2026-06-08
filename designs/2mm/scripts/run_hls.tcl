# Vitis HLS 2023.2 template for the 2mm project.

proc getenv_or_default {name default_value} {
  if {[info exists ::env($name)] && $::env($name) ne ""} {
    return $::env($name)
  }
  return $default_value
}

set top_function [getenv_or_default HLS_TOP "kernel_2mm"]
set variant_name [getenv_or_default HLS_VARIANT "baseline"]
set clock_period [getenv_or_default HLS_CLOCK_PERIOD_NS "10.0"]
set target_part [getenv_or_default HLS_PART "xc7k325tffv900-2"]
set src_files [getenv_or_default HLS_SRC_FILES "../src/kernel_2mm_baseline.cpp ../src/kernel_2mm.h"]
set tb_files [getenv_or_default HLS_TB_FILES "../tb/tb_kernel_2mm.cpp"]

if {$target_part ne "xc7k325tffv900-2"} {
  puts stderr "ERROR: This project is fixed to Vivado part xc7k325tffv900-2."
  puts stderr "ERROR: Refusing requested HLS_PART=$target_part"
  exit 1
}

open_project -reset "hls_${variant_name}"
set_top $top_function

foreach src_file $src_files {
  add_files $src_file
}

foreach tb_file $tb_files {
  add_files -tb $tb_file
}

open_solution -reset "solution1" -flow_target vivado
set_part $target_part
create_clock -period $clock_period -name default

csim_design
csynth_design
cosim_design
export_design -format ip_catalog

exit 0

