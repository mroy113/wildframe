# S0-12: configure-time fetch of pinned CR3 test fixtures.
#
# Satisfies NFR-2 (accuracy — tests exercise realistic inputs) and §11
# (large binary assets are not committed; they are fetched to build/).
# Mirrors the S0-11 models fetch pattern: pinned URL + SHA256, cached on
# re-run, FATAL_ERROR with a clear diagnostic on download or hash failure.
#
# Fixture categories and test intent live in docs/FIXTURES.md. Pin URLs
# and hashes are the version record for the test corpus — rotate with a
# new fixtures-vN release tag.

set(WILDFRAME_FIXTURES_DIR "${CMAKE_SOURCE_DIR}/build/_fixtures"
    CACHE PATH "Destination directory for fetched CR3 test fixtures (S0-12).")

file(MAKE_DIRECTORY "${WILDFRAME_FIXTURES_DIR}")

function(wildframe_fetch_fixture)
    cmake_parse_arguments(ARG "" "NAME;FILENAME;URL;SHA256" "" ${ARGN})
    foreach(required IN ITEMS NAME FILENAME URL SHA256)
        if(NOT ARG_${required})
            message(FATAL_ERROR "wildframe_fetch_fixture: missing required argument ${required}")
        endif()
    endforeach()

    set(target_path "${WILDFRAME_FIXTURES_DIR}/${ARG_FILENAME}")

    if(EXISTS "${target_path}")
        file(SHA256 "${target_path}" current_hash)
        if(current_hash STREQUAL ARG_SHA256)
            message(STATUS "wildframe: fixture ${ARG_NAME} cache hit (${ARG_FILENAME})")
            return()
        endif()
        message(STATUS "wildframe: fixture ${ARG_NAME} hash mismatch, re-downloading")
        file(REMOVE "${target_path}")
    endif()

    message(STATUS "wildframe: fetching fixture ${ARG_NAME} from ${ARG_URL}")
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
            "wildframe: failed to fetch fixture ${ARG_NAME}\n"
            "  URL:    ${ARG_URL}\n"
            "  Status: ${status_code} ${status_message}\n"
            "Check network access, then re-run configure. If the URL is\n"
            "permanently unreachable, update tools/fetch_fixtures.cmake with\n"
            "a new pin and matching SHA256."
        )
    endif()

    message(STATUS "wildframe: fetched fixture ${ARG_NAME} (${ARG_FILENAME})")
endfunction()

# Pinned fixture catalog. URL and SHA256 must be updated together; a stale
# pair is a bug. Category, test intent, and camera provenance live in
# docs/FIXTURES.md §2.
#
# Fixtures 4 (motion-blurred bird) and 5 (false-positive magnet) are not
# yet acquired — see docs/FIXTURES.md §4. Tests that depend on them should
# skip with a diagnostic until they land.

wildframe_fetch_fixture(
    NAME     "fixture_01 clear bird (FR-3, FR-4 golden path)"
    FILENAME "fixture_01_clear_bird.CR3"
    URL      "https://github.com/mroy113/wildframe/releases/download/fixtures-v1/fixture_01_clear_bird.CR3"
    SHA256   "6b474e6764c11e085bc8ce40c7fc39efd53b1603691e95fee4fb00512942d95e"
)

wildframe_fetch_fixture(
    NAME     "fixture_02 no bird (FR-3 negative path)"
    FILENAME "fixture_02_no_bird.CR3"
    URL      "https://github.com/mroy113/wildframe/releases/download/fixtures-v1/fixture_02_no_bird.CR3"
    SHA256   "e3b211bdfddc630362657f794b0b1a34834c03ffe36de7e455d4997fd9518e01"
)

wildframe_fetch_fixture(
    NAME     "fixture_03 small distant bird (FR-4 decode_crop fallback)"
    FILENAME "fixture_03_small_distant_bird.CR3"
    URL      "https://github.com/mroy113/wildframe/releases/download/fixtures-v1/fixture_03_small_distant_bird.CR3"
    SHA256   "edc9e1486f07fe7c375086ffcc4bb99942438b37dc84e95a826f5941bce7ae3f"
)
