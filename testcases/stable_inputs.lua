require "harness"

function run ()
  trig = TriggerWheel:new{primary_teeth=8}
  val = Validator:new{ignition={
    [0] = {0, 90, 180, 270, 360, 450, 540, 630}
   },
   wheel = trig
  }
  harness = Harness:new{trigger=trig, validator=val}
  
  trig:set_rpm(0, 6000)
  trig:set_rpm(1000, 2000)
  print(trig:get_angle(1))
  print(trig:get_angle(1.01))
  print(trig:get_angle(1.001))
  print(trig:get_angle(1000))
  print(trig:get_angle(1000.01))
  print(trig:get_angle(1000.001))
  return harness
  
end

run()
