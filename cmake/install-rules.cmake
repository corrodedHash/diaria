install(
    TARGETS
      diaria_cli
      # diaria_tui
    RUNTIME COMPONENT DIARIA_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
