#!/usr/bin/env bash

# afterdark.sh: Old-school screensaver functionality for GNOME/Wayland desktops
# This script will run arbitrary command after specified time period, if there is no inhibiting applications detected
# Started process will be terminated at the first sight of user activity
# You supposed to launch something like full-screen video or cmatrix :)

start_after=10 # minutes
cmd="xlock -nolock"
lock_screen=true

get_idle_minutes() {
  idle_time=$(dbus-send --print-reply --dest=org.gnome.Mutter.IdleMonitor \
    /org/gnome/Mutter/IdleMonitor/Core org.gnome.Mutter.IdleMonitor.GetIdletime | grep -Po '(?<=uint64\s)\d+')
  if [ -n "$idle_time" ]; then
    echo "$idle_time" / 60000 | bc
  else
    echo "0"
  fi
}

get_inhibitors() {
  for i in $(dbus-send --print-reply --dest=org.gnome.SessionManager \
    /org/gnome/SessionManager org.gnome.SessionManager.GetInhibitors | grep -Po 'Inhibitor\d+'); do
    if dbus-send --print-reply --dest=org.gnome.SessionManager \
      /org/gnome/SessionManager/"$i" org.gnome.SessionManager.Inhibitor.GetFlags | grep -q 'uint32\s8'; then
      true; return
    fi
  done
  false; return
}

while :; do
  if [ "$(get_idle_minutes)" -ge $start_after ] && [ -z "$screensaver" ]; then
    if ! get_inhibitors; then
      gnome-session-inhibit $cmd &
      screensaver=$!
    fi
  elif [ "$(get_idle_minutes)" -lt $start_after ] && [ -n "$screensaver" ]; then
    if $lock_screen; then
      dbus-send --type=method_call --dest=org.gnome.ScreenSaver \
        /org/gnome/ScreenSaver org.gnome.ScreenSaver.Lock
    fi
    pkill -P $screensaver
    screensaver=""
  fi
  sleep 1
done
