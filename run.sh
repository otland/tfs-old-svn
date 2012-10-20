#!/bin/bash

#The forgotten server run script

echo "The forgotten server run script"
VERSION="1.00a"

case "$1" in
  --help|-h)
      echo >&2 "Usage $0 <options>"
      echo >&2 " Where options are:"
      echo >&2 " -h|--help|start|stop|restart|kill|--version";
      if [ -f theforgottenserver ];
      then
        ./theforgottenserver --help
      fi
      ;;
   start|s)
     if [ -f theforgottenserver ];
     then
        echo >&2 "Running The forgotten server v$VERSION"
        ./theforgottenserver
        echo >&2 "Exited with code $?"
      else
         echo >&2 "Could not start The forgotten server"
      fi
     ;;
   restart)
     echo "Restarting..."
     killall -9 theforgottenserver
     if [ -f theforgottenserver ];
     then
        ./theforgottenserver
		echo >&2 "Exited with code $?"
     else
        echo >&2 "Failed"
     fi
     ;;
   stop|kill)
     killall -9 theforgottenserver
     ;;
   --version)
     echo "v$VERSION"
     exit
     ;;
   *)
      echo >&2 "Usage $0 <options>"
      echo >&2 " Where options are:"
      echo >&2 " -h|--help|start|stop|restart|kill";
      ;;
esac
