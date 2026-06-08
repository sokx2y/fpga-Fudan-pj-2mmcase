# Check that Vivado 2023.2 recognizes the fixed project part.

set required_part "xc7k325tffv900-2"
set part_pattern "*xc7k325t*900*"
set available_parts [lsort [get_parts $part_pattern]]

if {[lsearch -exact $available_parts $required_part] < 0} {
  puts stderr "ERROR: Vivado does not recognize required part: $required_part"
  puts stderr "Available parts from: get_parts $part_pattern"

  if {[llength $available_parts] == 0} {
    puts stderr "  <none>"
  } else {
    foreach part $available_parts {
      puts stderr "  $part"
    }
  }

  exit 1
}

puts "OK: Vivado recognizes required part: $required_part"
exit 0
