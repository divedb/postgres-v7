#!/bin/bash

ipcs -s | awk '{print $2}' | while read id; do
    if [[ ${id} =~ ^[0-9]+$ ]]; then
        ipcrm -s ${id}
    fi
done