install(
    TARGETS diaria_cli
    RUNTIME COMPONENT DIARIA_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
