What's New
----------

 * Fixed bug in clear-tube not clearing after restart
 * Fixed bug in log load where group reload was attempted while not supported.
 * Added removal of group reference from job on group delete
 * Fixed add_job_to_group and a bug in put. Also made put group optional
 * Reduced some unnecessary code from do_stats
 * Optimised do_stats to only recall the fmt function if need be
