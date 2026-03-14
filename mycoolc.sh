#!/bin/sh

FILENAME=$(basename "$1" .cl)

ASSIGNMENTS_DIR="/usr/class/assignments"

$ASSIGNMENTS_DIR/PA2/lexer $1 | $ASSIGNMENTS_DIR/PA3/parser | $ASSIGNMENTS_DIR/PA4/semant | $ASSIGNMENTS_DIR/PA5/cgen > "$FILENAME.s"
