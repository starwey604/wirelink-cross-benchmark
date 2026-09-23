version 1;
message SetpointRequest @id(100) {
  required fixed32 request_id @id(1);
  required fixed32 channel @id(2);
  required fixed32 value @id(3);
}
message SetpointResponse @id(101) {
  required fixed32 request_id @id(1);
  required fixed32 status @id(2);
  required fixed32 applied_value @id(3);
}
message Sample @id(102) {
  required fixed32 sequence @id(1);
  required fixed32 sample_tick @id(2);
  required fixed32 value @id(3);
  required fixed32 flags @id(4);
}
