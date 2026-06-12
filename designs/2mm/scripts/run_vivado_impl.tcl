# Vivado 2023.2 implementation template for exported HLS RTL.

proc getenv_or_default {name default_value} {
  if {[info exists ::env($name)] && $::env($name) ne ""} {
    return $::env($name)
  }

  return $default_value
}

set fixed_part "xc7k325tffv900-2"
set variant_name [getenv_or_default VARIANT_NAME "baseline"]
set clock_period [getenv_or_default CLOCK_PERIOD_NS "10.0"]
set rtl_path [getenv_or_default RTL_PATH "../variants/${variant_name}/rtl"]
set xci_files [getenv_or_default XCI_FILES ""]
set top_module [getenv_or_default RTL_TOP "kernel_2mm"]
set place_directive [getenv_or_default PLACE_DIRECTIVE "Default"]
set phys_opt_directive [getenv_or_default PHYS_OPT_DIRECTIVE "Default"]
set route_directive [getenv_or_default ROUTE_DIRECTIVE "Default"]

create_project -force "vivado_${variant_name}" "vivado_${variant_name}" -part $fixed_part

set rtl_files {}
foreach pattern {"*.v" "*.sv"} {
  set rtl_files [concat $rtl_files [glob -nocomplain -directory $rtl_path $pattern]]
}

if {[llength $rtl_files] == 0 && $xci_files eq ""} {
  puts stderr "ERROR: No RTL files found under $rtl_path and no XCI_FILES provided"
  exit 1
}

if {[llength $rtl_files] > 0} {
  add_files $rtl_files
}

foreach xci_file $xci_files {
  add_files $xci_file
}

update_compile_order -fileset sources_1

if {[llength [get_ports -quiet ap_clk]] == 0} {
  puts stderr "ERROR: Cannot find ap_clk port for clock constraint."
  exit 1
}

create_clock -period $clock_period -name default [get_ports ap_clk]

synth_design -top $top_module -part $fixed_part
report_timing_summary -file "timing_post_synth_${variant_name}.rpt"
report_utilization -file "utilization_post_synth_${variant_name}.rpt"
opt_design

if {$place_directive eq "Default"} {
  place_design
} else {
  place_design -directive $place_directive
}

report_design_analysis -congestion -file "congestion_post_place_${variant_name}.rpt"

if {$phys_opt_directive eq "Default"} {
  phys_opt_design
} else {
  phys_opt_design -directive $phys_opt_directive
}

if {$route_directive eq "Default"} {
  route_design
} else {
  route_design -directive $route_directive
}

report_timing_summary -file "timing_post_route_${variant_name}.rpt"
report_utilization -file "utilization_post_route_${variant_name}.rpt"
report_route_status -file "route_status_post_route_${variant_name}.rpt"

set timing_paths [get_timing_paths -max_paths 1 -nworst 1]
if {[llength $timing_paths] == 0} {
  puts stderr "ERROR: No timing paths found after route_design."
  exit 1
}

set wns [get_property SLACK [lindex $timing_paths 0]]
if {$wns < 0} {
  puts stderr "ERROR: Timing failed. WNS=$wns"
  exit 1
}

puts "OK: Implementation timing met. WNS=$wns"
exit 0
