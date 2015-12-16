#!/bin/sh
# simple script that will fire set of events so daemon window positioning (and other) can be tested

N_EVENTS=5

#notify_cmd="notify-send --expire-time=3000"
#
#for((i=0;i<$N_EVENTS;i++)) do
#	$notify_cmd "title $i" "this is content"
#done

#method call sender=:1.798 -> dest=org.freedesktop.Notifications serial=116 path=/org/freedesktop/Notifications; interface=org.freedesktop.Notifications; member=Notify
#   string "Mumble"
#   uint32 1
#   string "gtk-dialog-info"
#   string "User Joined Channel"
#   string "<a href='clientid://df93eabe004245bd18dd38cf9a07bd5242b2f522' class='log-user log-target'>madamova</a> entered channel."
#   array [
#   ]
#   array [
#      dict entry(
#         string "desktop-entry"
#         variant             string "mumble"
#      )
#   ]
#   int32 -1

dbus-send --type=method_call \
	--dest=org.freedesktop.Notifications \
	/org/freedesktop/Notifications org.freedesktop.Notifications.Notify \
	string:"Mumble" \
	uint32:1 \
	string:"gtk-dialog-info" \
	string:"User Joined Channel" \
	string:"<a href='clientid://df93eabe004245bd18dd38cf9a07bd5242b2f522' class='log-user log-target'>madamova</a> entered channel." \
	array:string:'' \
	dict:string:string:'','' \
	int32:-1

sleep 1

dbus-send --type=method_call \
	--dest=org.freedesktop.Notifications \
	/org/freedesktop/Notifications org.freedesktop.Notifications.Notify \
	string:"Mumble 2" \
	uint32:1 \
	string:"gtk-dialog-info" \
	string:"User Joined Channel #2" \
	string:"<a href='clientid://df93eabe004245bd18dd38cf9a07bd5242b2f522' class='log-user log-target'>madamova</a> entered channel. asdasd asdasdsa asdasd asdasd asdasdsa" \
	array:string:'' \
	dict:string:string:'','' \
	int32:-1

sleep 1

dbus-send --type=method_call \
	--dest=org.freedesktop.Notifications \
	/org/freedesktop/Notifications org.freedesktop.Notifications.Notify \
	string:"Mumble 2" \
	uint32:1 \
	string:"gtk-dialog-info" \
	string:"User Joined Channel #2 dasdasd asdasfdsa fsdfsdfa sdfsdfasdf asdfsdfasdfasdfasdfa asdfasdfdsafasdfasdfdsafdsaf sadfsdafds" \
	string:"doh!" \
	array:string:'' \
	dict:string:string:'','' \
	int32:-1
