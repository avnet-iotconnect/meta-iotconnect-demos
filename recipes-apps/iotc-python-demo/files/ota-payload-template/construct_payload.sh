#!/bin/bash
tar --exclude=".gitkeep" --exclude="construct_payload.sh" --exclude="README" -czvf ota-payload.tar.gz ./