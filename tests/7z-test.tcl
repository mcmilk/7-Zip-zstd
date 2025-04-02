#!/usr/bin/env tclsh

# 7z-test.tcl --
#
# Test suite to test 7-zip ZS.
#
# Copyright (c) 2025- by Sergey G. Brester aka sebres

package require tcltest
namespace import ::tcltest::*

if {[namespace which -command "::7z"] eq ""} {
  source [file join [file dirname [info script]] 7z.tcl]
}

configure -testdir [file normalize [file dirname [info script]]] -singleproc 1 -tmpdir $::env(TEMP) {*}$argv

if {[runAllTests]} {return -code error "FAILED"}
