#!/bin/bash

if [ ! -d "${HOME}/.hashahead/" ]; then
    mkdir -p ${HOME}/.hashahead/
fi

if [ ! -f "${HOME}/.hashahead/hashahead.conf" ]; then
    cp /hashahead.conf ${HOME}/.hashahead/hashahead.conf
fi

exec "$@"
