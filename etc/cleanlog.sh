#!/bin/sh

HURUH="/home/huru"

cd $HURUH
rm -rf auth/log/* vault/log/* lobby/log/* tracking/log/* lobby/*/log/*
rm -f auth/CGASauth.log
