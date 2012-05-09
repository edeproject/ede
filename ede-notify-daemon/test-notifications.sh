#!/bin/sh
# simple script that will fire set of events so daemon window positioning (and other) can be tested

N_EVENTS=5

notify_cmd="notify-send --expire-time=10000"

for((i=0;i<$N_EVENTS;i++)) do
	$notify_cmd "title $i" "this is content"
done
