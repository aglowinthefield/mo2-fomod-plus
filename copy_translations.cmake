# Copy all .qm files from build directories to package/translations
file(GLOB_RECURSE QM_FILES "${SOURCE_DIR}/*.qm")

foreach(QM_FILE ${QM_FILES})
    get_filename_component(FILENAME ${QM_FILE} NAME)
    file(COPY_FILE ${QM_FILE} "${DEST_DIR}/${FILENAME}")
    message(STATUS "Copied ${FILENAME}")
endforeach()