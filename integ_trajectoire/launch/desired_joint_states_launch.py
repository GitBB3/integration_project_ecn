from simple_launch import SimpleLauncher

sl = SimpleLauncher(use_sim_time = True)


def launch_setup():

    sl.node('integ_trajectoire','desired_joint_states')
                        

    return sl.launch_description()

generate_launch_description = sl.launch_description(launch_setup)
