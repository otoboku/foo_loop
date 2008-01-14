#!/bin/zsh
# XXX target: MAC OS X Leopard.
rm wassr.webapp
zip wassr.webapp -j src/*
unzip -l wassr.webapp
open /Applications/Prism.app wassr.webapp

