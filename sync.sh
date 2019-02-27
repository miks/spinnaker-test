REMOTE_DIR=${REMOTE_DIR:-spinnaker-capture}
START=${START:-0}
COMPILE_COMMAND="cd ${REMOTE_DIR} && make"

if [ "${START}" == "1" ] ; then
  COMPILE_COMMAND="${COMPILE_COMMAND} && ./bin/capture"
fi

# sync
rsync -rcv --delete --force --ignore-times --exclude-from .gitignore --exclude '.*' --exclude '*.sh' ./ ${SSH_HOST}:${REMOTE_DIR}
# compile
ssh -tt ${SSH_HOST} "${COMPILE_COMMAND}"
