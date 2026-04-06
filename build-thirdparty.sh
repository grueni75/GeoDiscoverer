source ~/.androidrc
git submodule update --init --recursive
sh -c "cd ThirdParty/mapsforge && ./build.sh"
sh -c "cd ThirdParty/acra && ./build.sh"
sh -c "cd ThirdParty/brouter && ./build.sh"

