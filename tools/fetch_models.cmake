# S0-11: configure-time fetch of pinned ONNX model weights.
#
# Satisfies handoff §11 (model weights are not committed) and §15 (licensing
# audit trail — pinned URL + SHA256 are the version record per weight).
#
# Idempotent: if the destination file already exists with the expected hash,
# no network traffic. A hash mismatch or download failure is FATAL_ERROR
# during configure, with a diagnostic that points at this file.

set(WILDFRAME_MODELS_DIR "${CMAKE_SOURCE_DIR}/build/_models"
    CACHE PATH "Destination directory for fetched ONNX model weights (S0-11).")

file(MAKE_DIRECTORY "${WILDFRAME_MODELS_DIR}")

function(wildframe_fetch_model)
    cmake_parse_arguments(ARG "" "NAME;FILENAME;URL;SHA256" "" ${ARGN})
    foreach(required IN ITEMS NAME FILENAME URL SHA256)
        if(NOT ARG_${required})
            message(FATAL_ERROR "wildframe_fetch_model: missing required argument ${required}")
        endif()
    endforeach()

    set(target_path "${WILDFRAME_MODELS_DIR}/${ARG_FILENAME}")

    if(EXISTS "${target_path}")
        file(SHA256 "${target_path}" current_hash)
        if(current_hash STREQUAL ARG_SHA256)
            message(STATUS "wildframe: model ${ARG_NAME} cache hit (${ARG_FILENAME})")
            return()
        endif()
        message(STATUS "wildframe: model ${ARG_NAME} hash mismatch, re-downloading")
        file(REMOVE "${target_path}")
    endif()

    message(STATUS "wildframe: fetching model ${ARG_NAME} from ${ARG_URL}")
    file(DOWNLOAD
        "${ARG_URL}"
        "${target_path}"
        EXPECTED_HASH SHA256=${ARG_SHA256}
        TLS_VERIFY ON
        SHOW_PROGRESS
        STATUS download_status
    )

    list(GET download_status 0 status_code)
    list(GET download_status 1 status_message)
    if(NOT status_code EQUAL 0)
        file(REMOVE "${target_path}")
        message(FATAL_ERROR
            "wildframe: failed to fetch model ${ARG_NAME}\n"
            "  URL:    ${ARG_URL}\n"
            "  Status: ${status_code} ${status_message}\n"
            "Check network access, then re-run configure. If the URL is\n"
            "permanently unreachable, update tools/fetch_models.cmake with\n"
            "a new pin and matching SHA256."
        )
    endif()

    message(STATUS "wildframe: fetched model ${ARG_NAME} (${ARG_FILENAME})")
endfunction()

# Pinned model catalog. URL and SHA256 must be updated together; a stale pair
# is a bug. Provenance (exporter toolchain + command) lives in docs/LICENSING.md §3.
wildframe_fetch_model(
    NAME     "YOLOv11n (COCO-80 detection, FR-3)"
    FILENAME "yolo11n.onnx"
    URL      "https://github.com/mroy113/wildframe/releases/download/models-v1/yolo11n.onnx"
    SHA256   "119040d6483aee48c168b427bf5d383d6cadde5745c6991e6dc24eeabf07a05a"
)
