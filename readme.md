install:
libao-dev
libmpg123-dev


compiler:

gcc -o fossodoro fossodoro.c $(pkg-config --cflags --libs gtk+-3.0 libnotify lmpg123 -lao)

xgettext --package-name fossodoro --package-version 1.2 --default-domain fossodoro --output fossodoro.pot pomodoro.c 

msgfmt -o locale/pt_BR/LC_MESSAGES/fossodoro.mo pt_BR.po