#/bin/sh
tar czvf phantom-cov.tgz cov-int
curl --form token=$COVERITY_TOKEN \
  --form email=dz@dz.ru \
  --form file=@phantom-cov.tgz \
  --form version="Commit1668" \
  --form description="Phantom OS build" \
  https://scan.coverity.com/builds?project=dzavalishin%2Fphantomuserland
