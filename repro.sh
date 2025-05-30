#!/bin/bash

export BIGMEMBENCH_COMMON_PATH="/ssd1/asplos25_ae/hybridtier-asplos25-artifact"

./repro_hybridtier.sh
# memtis experiments will automatically be launched after hybrdtier experiments are done
# postprocess results will run after all results are done
