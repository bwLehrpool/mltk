#!/usr/bin/python
#
# Author: Matt Fischer <matthew.fischer@canonical.com>
# Copyright (C) 2012 Canonical, Ltd
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version. See http://www.gnu.org/copyleft/gpl.html the full text of the
# license.
#
# This code is based on the LightDM GTK Greeter which was written by:
# Robert Ancell <robert.ancell@canonical.com>

# required packages:
# liblightdm-gobject-1-0
# gir1.2-lightdm-1
# python-gobject
# gir1.2-glib-2.0
# gir1.2-gtk-3.0

from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import GdkPixbuf
from gi.repository.GdkPixbuf import InterpType
from gi.repository import LightDM
import sys

greeter = None

main = None
split = None

guest_box = None
guest_text = None
guest_button = None
guest_image = None

login_box = None
login_text = None
user_box = None
user_text = None
user_username = None
pass_box = None
pass_text = None
pass_password = None

prompt_box = None
prompt_label = None
prompt_entry = None
message_label = None

# This Gtk signal is called when the user hits enter after entering a
# username/password or clicks the login button.  Since we re-purposed
# the text entry box, we have 3 possible cases to handle here.
# 1) the user is already authenticated, if for example, they don't have
#  a password set.
# 2) The username has been passed into LightDM and now we need to pass
#  the password
# 3) The username has been entered, but not passed in.  We pass it in
#  and start the authentication process.
def login_cb(widget):
    print >> sys.stderr, "login_cb"
    if greeter.get_is_authenticated():
        print >> sys.stderr, "user is already authenticated, starting session"
        #start_session()
    elif greeter.get_in_authentication():
        print >> sys.stderr, "username was passed in already, send password to LightDM"
        print >> sys.stderr, greeter.get_authentication_user()
        greeter.respond(pass_password.get_text())
    else:
        print >> sys.stderr, "Initial entry of username, send it to LightDM"
        greeter.authenticate(user_username.get_text())

def guest_cb(widget):
    print >> sys.stderr, "guest_cb"
    if greeter.get_has_guest_account_hint():
        print >> sys.stderr, "Guest accounts supported"
        greeter.authenticate_as_guest()
    else:
        print >> sys.stderr, "Guest accounts not supported"

    
# Gtk Signal Handlers
handlers = {
    "login_cb": login_cb,
    "guest_cb": guest_cb
}

# The show_prompt callback is oddly named, but when you get this
# callback you are supposed to send the password to LightDM next.  In
# our example, we re-purpose the prompt and ask the user for the
# password which is then sent the next time the user hits the Login
# button or presses enter.
def show_prompt_cb(greeter, text, promptType):
    print >> sys.stderr, "Prompt type: " + str(promptType)
    print >> sys.stderr, "Text: " + str(text)
    # if this is a password prompt, we want to hide the characters
    if promptType == LightDM.PromptType.SECRET:
        pass_password.set_visibility(False)
    else:
        pass_password.set_visibility(True)
    greeter.respond(pass_password.get_text())


# If LightDM sends a message back to the greeter, for example, "Login
# failed" or "invalid password" we display it in our message box.
def show_message_cb(text, message_type):
    print >> sys.stderr, "In show_message"
    print >> sys.stderr, text
    message_label.set_text(text)
    message_label.show()

# Callback for after we send LightDM the password, this method
# has to handle a successful login, in which case we start the session
# or a failed login, in which case we tell the user
def authentication_complete_cb(greeter):
    if greeter.get_is_authenticated():
        # For our simple example we always start Unity-2d.  The LightDM
        # API has ways to query available sessions, please see the docs.
        if not greeter.start_session_sync("xfce"):
            print >> sys.stderr, "Failed to start session"
    else:
        print >> sys.stderr, "Login failed"
	message_label.set_text("LOGIN FAILED")
	message_label.show()

if __name__ == '__main__':
    print >> sys.stderr, "Starting up..."
    main_loop = GObject.MainLoop ()
    builder = Gtk.Builder()
    greeter = LightDM.Greeter()
    styler = Gtk.CssProvider()
    css = open('/usr/local/share/lightdm/netpoint.css', 'r')

    css_data = css.read()
    css.close()
    styler.load_from_data(css_data)
    Gtk.StyleContext.add_provider_for_screen(
        Gdk.Screen.get_default(),
        styler,
        Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
    )

    # connect signal handlers to LightDM
    greeter.connect ("authentication-complete", authentication_complete_cb)
    greeter.connect ("show-message", show_message_cb)
    greeter.connect ("show-prompt", show_prompt_cb)

    # connect builder and widgets
    # you probably really want to put your .UI file somewhere else
    builder.add_from_file("/usr/local/share/lightdm/netpoint.glade")

    main = builder.get_object("main")
    split = builder.get_object("split")
    sep = builder.get_object("sep")
    message_label = builder.get_object("message_label")
    print >> sys.stderr, message_label
    guest_box = builder.get_object("guest_box")
    
    guest_outer_box = builder.get_object("guest_outer_box")
    guest_internal_box = builder.get_object("guest_internal_box")
    guest_text = builder.get_object("guest_text")
    guest_button = builder.get_object("guest_button")
    guest_image = builder.get_object("guest_image")
    login_image = builder.get_object("login_image")
    #pixbuf = guest_image.get_pixbuf()
    #pixbuf = pixbuf.scale_simple(pixbuf.get_width() * 0.7, pixbuf.get_height() * 0.7, InterpType.HYPER)
    #guest_image.set_from_pixbuf(pixbuf)
    #pixbuf = login_image.get_pixbuf()
    #pixbuf = pixbuf.scale_simple(pixbuf.get_width() * 0.7, pixbuf.get_height() * 0.7, InterpType.HYPER)
    #login_image.set_from_pixbuf(pixbuf)

    login_box = builder.get_object("login_box")
    login_text = builder.get_object("login_text")

    user_box = builder.get_object("user_box")
    user_text = builder.get_object("user_text")
    user_username = builder.get_object("user_username")

    pass_box = builder.get_object("pass_box")
    pass_text = builder.get_object("pass_text")
    pass_password = builder.get_object("pass_password")

    # connect signals to Gtk UI
    builder.connect_signals(handlers)

    # connect to greeter
    greeter.connect_sync()

    message_label.hide()

    # setup the GUI
    main.set_decorated(True)
    main.get_root_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.ARROW))
    main.show()
    guest_text.show()
    guest_button.show()
    login_text.show()
    user_text.show()
    user_username.grab_focus()
    user_username.show()
    pass_text.show()
    pass_password.set_sensitive(True)
    pass_password.set_visibility(False)
    pass_password.show()

    # fullscreen it
    main.resize(Gdk.Screen.width(), Gdk.Screen.height())

    #print >> sys.stderr, guest_box.get_height()
    #print >> sys.stderr, guest_box.get_width()
    main_loop.run ()
