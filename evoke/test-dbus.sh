
#dbus-send --session --print-reply --dest=org.equinoxproject.Xsettings \
#                  /org/freedesktop/Xsettings        \
#                  org.freedesktop.Xsettings.GetType \
#                  string:"Gtk/CanChangeAccels"

#dbus-send --session --print-reply --dest=org.equinoxproject.Xsettings \
#                  /org/freedesktop/Xsettings        \
#                  org.freedesktop.Xsettings.GetAll

#dbus-send --session --print-reply --dest=org.equinoxproject.Xsettings \
#                  /org/freedesktop/Xsettings        \
#                  org.freedesktop.Xsettings.GetValue \
#				  string:"Fltk/Background"

dbus-send --session --print-reply --dest=org.equinoxproject.Xsettings \
                  /org/freedesktop/Xsettings        \
                  org.freedesktop.Xsettings.Set \
				  string:"Sanel/Test" int32:34

#dbus-send --session --print-reply --dest=org.equinoxproject.Xsettings \
#                  /org/freedesktop/Xsettings        \
#                  org.freedesktop.Xsettings.Flush

#dbus-send --session --print-reply --dest=org.equinoxproject.Xsettings \
#                  /org/freedesktop/Xsettings        \
#                  org.freedesktop.Xsettings.Remove \
#				  string:"Bell/Pitch"
