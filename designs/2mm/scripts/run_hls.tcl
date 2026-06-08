# Vitis HLS 2023.2 template for the 2mm project.

proc getenv_or_default {name default_value} {
  if {[info exists ::env($name)] && $::env($name) ne ""} {
    return $::env($name)
  }

  return $default_value
}

proc getenv_bool {name default_value} {
  set raw [getenv_or_default $name $default_value]
  set value [string tolower $raw]

  return [expr {$value in {"1" "true" "yes" "on"}}]
}

set fixed_part "xc7k325tffv900-2"
set top_function [getenv_or_default HLS_TOP "kernel_2mm"]
set variant_name [getenv_or_default HLS_VARIANT "baseline"]
set clock_period [getenv_or_default HLS_CLOCK_PERIOD_NS "10.0"]
set src_files [getenv_or_default HLS_SRC_FILES "../src/kernel_2mm_baseline.cpp ../src/kernel_2mm.h"]
set tb_files [getenv_or_default HLS_TB_FILES "../tb/tb_kernel_2mm.cpp"]
set run_cosim [getenv_bool HLS_RUN_COSIM "0"]
set export_rtl [getenv_bool HLS_EXPORT_RTL "0"]

open_project -reset "hls_${variant_name}"
set_top $top_function

foreach src_file $src_files {
  add_files $src_file
}

foreach tb_file $tb_files {
  add_files -tb $tb_file
}

open_solution -reset "solution1" -flow_target vivado
set_part $fixed_part
create_clock -period $clock_period -name default

csim_design
csynth_design

if {$run_cosim} {
  cosim_design
}

if {$export_rtl} {
  export_design -format ip_catalog
}

exit 0
