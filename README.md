# Robocon-2022

## Design of Seeker Robot 1 (Seeker R1)

![](https://github.com/Patil-Vinay/Robocon-2022/blob/main/R1_Robot_Design.png)

The Robot R1’s body contains the Hopper-Funnel mechanism to accept all the three balls at once from the Robot R2. The central ball of the three gets loaded first and the side ones are held in place by a hopper mechanism consisting of two stepper motors and will be responsible for reloading. The other balls will be loaded one by one when the previous one is shot at the target lagori.

The Robot R1’s body consists of two linear actuators at its base plate that help lift the whole rotating body (pipe-shooter mechanism) and provide an angle of elevation to it. The ball, already loaded in the pipe-shooter is then pushed gently towards the high-RPM motors, where the motors depict the mechanism of a baseball shooter machine and shoots the ball at the target lagori by calculating the trajectory and range using image detection. 

## Design of Picker Robot 2 (Picker R2)

![](https://github.com/Patil-Vinay/Robocon-2022/blob/main/R2_Robot_Design.png)

For picking up the lagori and carrying them to lagori’s square base, we are using three suction cups and vacuum. There are two suction cups(i.e Suction Cup 1 and 2) facing downwards and one suction cup(i.e Suction Cup 3) facing sideways. We will use sideways suction cup (i.e Suction Cup 3) to lift only the topmost lagori to the lagori pile. We will be creating a vacuum inside the suction cup by placing these suction cups on lagori’s, which will hold the lagori’s in place and by releasing the vacuum the lagori’s will be released. These suction cups are connected to a platform via linear actuators which can extend outwards. These suction cups which are attached to the platform can move linearly up and down using stepper motors. When R2 gets close to the lagori pile or lagori base, the platform will travel linearly upward using stepper motors and come to a stop at a height where lagori can be lowered to the pile. After reaching the desired height, the linear actuators will extend themselves to align lagori right above the lagori pile or lagori base and then it will drop the lagori’s by releasing the vacuum in the suction cup.

For picking up all three balls at once and carrying them to R1, we are using three suction cups and vacuum. The robot uses a vacuum pump to create a vacuum inside the suction cup to catch all three balls, which will hold the ball in place and by releasing the vacuum all the balls will be released. These three suction cups are connected to a platform via linear actuators which extend outwards and are attached to the platform which can move linearly up and down using stepper motors. When R2 gets close to R1, the platform will travel linearly upward using stepper motors and come to a stop at a height where balls can be lowered to the R1 feeder. After reaching the desired height, the linear actuators will extend themselves to align balls right above the R1 feeder and then it will drop all the three balls by releasing the vacuum in the suction cup.
