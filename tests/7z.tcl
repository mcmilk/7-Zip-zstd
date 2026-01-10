# 7z.tcl --
#
# Helper for the 7-Zip ZS test suite.
#
# Copyright (c) 2025- by Sergey G. Brester aka sebres

if {[namespace which -command "::test"] eq ""} {
	package require tcltest
	namespace import -force ::tcltest::*
}

variable Z7_PATH
variable Z7_REGR_TEST_DIR [file join [file dirname [info script]] regr-arc]
variable Z7_PWD "very-secret-pwd"

if {[info exists ::env(Z7_PATH)]} {
	set Z7_PATH $::env(Z7_PATH)
}
if {![info exists Z7_PATH]} {
	apply {{} {
		variable Z7_PATH
		foreach p {bin--x64 bin--x86} {
			set Z7_PATH [file join [file dirname [file dirname [info script]]] bin $p 7z.exe]
			if {[file exists $Z7_PATH]} break
		}
	}}
}
puts "Test 7z-path: $Z7_PATH"

# ensure we can test using executable from $Z7_PATH (valid platform, e. g. bypass cross-platform build):
apply {{} {
	variable Z7_PATH
	if {[catch {exec $Z7_PATH} res opt]} {
		puts "Cannot test using \"$Z7_PATH\" (invalid platform?): $res"
		# 0 - by "invalid argument" (invalid platform), 1 - by other (unexpected) error
		exit [expr {![string match {POSIX EINVAL *} [dict get $opt -errorcode]]}]
	}
}}

proc 7z {args} {
	variable Z7_PATH
	variable Last7zLog
	set Last7zLog [exec $Z7_PATH {*}$args]
}

proc 7z_2_bin {args} {
	variable Z7_PATH
	set f [open [list | $Z7_PATH {*}$args] rb]
	try {
		set b [read $f]
	} finally {
		close $f
	}
	return $b
}

proc 7z_get_info {args} {
	set res [7z l -slt {*}$args]
	set ret {}
	# archive path and type:
	lappend ret {*}[lrange [regexp -inline -expanded {
		\n--
		\n(Path)\s*=\s*[^\n]*[\\/]([^\\/\n]+)
		\n(Type)\s*=\s*(\S*)
		(?:\n(?!Solid)[^\n]*){0,5}
		(?:\n(Solid)\s*=\s*(\S+))?
	} $res] 1 end]
	if {[lindex $ret end] eq {}} {set ret [lreplace $ret end-1 end]}
	# info of content files:
	set i 0
	set sepRE {\n-{5,}}
	set flst {}
	while {[regexp -start [lindex $i end] -indices $sepRE $res i]} {
		set fi {}
		foreach {n v} [lrange [regexp -start [lindex $i 1] -inline -expanded {
			(?:\n(?!Path)[^\n]*){0,5}
			(?:\n(Path)\s*=\s*(\S+))
			(?:\n(?!Size)[^\n]*){0,5}
			(?:\n(Size)\s*=\s*(\S+))?
			(?:\n(?!Modified)[^\n]*){0,5}
			(?:\n(Modified)\s*=\s*([^\n]+))?
			(?:\n(?!CRC)[^\n]*){0,5}
			(?:\n(CRC)\s*=\s*(\S+))?
			(?:\n(?!Method)[^\n]*){0,5}
			(?:\n(Method)\s*=\s*(\S+))?
		} $res] 1 end] {
			if {$n eq ""} continue
			if {$n eq "Modified"} {
				# to unix time (UTC, TZ independend)
				set v [clock scan [regsub {\.\d+$} $v {}]]
			}
			if {$n eq "Method"} {
				# remove version from method (unneeded and expecting adjustment of all tests by later version upgrades):
				regsub -all {v\d+\.\d+,?} $v {} v
			}
			lappend fi $n $v
		}
		if {[llength $fi]} { lappend flst $fi }
		set sepRE {\n\n}
	}
	if {[llength $flst]} { lappend ret Files $flst }
	return $ret
}

proc assertLogged {args} {
	variable Last7zLog
	foreach re $args {
		if {![regexp $re $Last7zLog]} {
			return -code error "$re cannot be found in last std-output:\n[string repeat = 40]\n$Last7zLog\n[string repeat = 40]\n"
		}
	}
}
