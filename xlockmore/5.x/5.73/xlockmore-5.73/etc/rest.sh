#!/bin/bash
#
# Force me to take rest breaks
#

set -eu

ACTIVITY=900
REST=30

while true; do
        sleep "$ACTIVITY"

        xlock &

        sleep "$REST"

	# xlock needs to obey SIGINT to be useful
        kill %1 2> /dev/null

        wait

done
