# SPDX-License-Identifier: GPL-3.0-or-later
add_executable(run_sql "run_sql.cpp")
setup_target(TARGET run_sql LIBRARIES util libpqxx::pqxx)
