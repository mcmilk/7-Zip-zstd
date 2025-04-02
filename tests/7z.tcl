# 7z.tcl --
#
# Helper for the 7-zip ZS test suite.
#
# Copyright (c) 2025- by Sergey G. Brester aka sebres

if {[namespace which -command "::test"] eq ""} {
  package require tcltest
  namespace import -force ::tcltest::*
}

variable 7Z_PATH
variable 7Z_REGR_TEST_DIR [file join [file dirname [info script]] regr-arc]

if {[info exists ::env(7Z_PATH)]} {
  set 7Z_PATH $::env(7Z_PATH)
}
if {![info exists 7Z_PATH]} {
  apply {{} {
    variable 7Z_PATH
    foreach p {bin--x64 bin--x86} {
      set 7Z_PATH [file join [file dirname [file dirname [info script]]] bin $p 7z.exe]
      if {[file exists $7Z_PATH]} break
    }
  }}
}
puts "Test 7z-path: $7Z_PATH"
if {[catch {exec $7Z_PATH} res]} {
  puts "Cannot test using \"$7Z_PATH\" (invalid platform?): $res"
  exit 0
}

proc 7z {args} {
  variable 7Z_PATH
  variable Last7zLog
  set Last7zLog [exec $7Z_PATH {*}$args]
}

proc assertLogged {args} {
  variable Last7zLog
  foreach re $args {
    if {![regexp $re $Last7zLog]} {
      return -code error "$re cannot be found in last std-output:\n[string repeat = 40]\n$Last7zLog\n[string repeat = 40]\n"
    }
  }
}