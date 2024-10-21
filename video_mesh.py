import pyray as rl
from pyray import KeyboardKey as K
from pygame import mixer
import time
import cv2
import math
import os
import sys
from enum import Enum

class MouseState(Enum):
    CAPTURED=0
    ACTIVE=1
    
WIDTH = 640
HEIGHT = 480
GRAVITY = 200
PLAYER_SIZE = 10
global FILE
FILE = None


def get_image_from_frame(frame: cv2.Mat) -> rl.Image:
    height = frame.shape[0]
    width = frame.shape[1]
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    image = rl.image_copy(rl.Image(frame.tobytes(), width, height, 1, rl.PixelFormat.PIXELFORMAT_UNCOMPRESSED_R8G8B8))
    return image
    
def project(a, b) -> rl.Vector3:
    value = (rl.vector_3dot_product(a, b)/rl.vector_3dot_product(b, b))
    return rl.vector3_scale(b, value)
    
def main():
    global FILE
    if len(sys.argv) > 1:
        FILE = sys.argv[1]
        
    mixer.init()
    rl.init_window(1280, 720, "THING")
    if FILE:
        video = cv2.VideoCapture(FILE)
    else:
        video = cv2.VideoCapture(0)
      
    fps = video.get(cv2.CAP_PROP_FPS)
    if FILE:
        rl.set_target_fps(int(fps))
    
    if FILE and not os.path.isfile(f"{FILE[:FILE.index('.')]}.mp3"):
        os.system(f"ffmpeg -i {FILE} -map 0:a {FILE[:FILE.index('.')]}.mp3")
    if FILE:
        mixer.music.load(f"{FILE[:FILE.index('.')]}.mp3")
        mixer.music.set_volume(0.4)

    mario = rl.load_image("res/mario.png")
    mario_text = rl.load_texture_from_image(mario)
    mario_plane: rl.Model = rl.load_model_from_mesh(rl.gen_mesh_plane(WIDTH, WIDTH, 10, 10))
    mario_plane.materials[0].maps[0].texture = mario_text

    max_height = 80.0
    max_height_ptr = rl.ffi.new('float *', max_height)  # Initial value 80.0
    camera: rl.Camera3D = rl.Camera3D(rl.Vector3(0,0,0), rl.Vector3(0, 0,-1), 
                rl.Vector3(0, 1, 0), 60.0, rl.CameraProjection.CAMERA_PERSPECTIVE)
    
    wall_positions = [rl.Vector3(0,0,-WIDTH/2),rl.Vector3(0,0, WIDTH/2),
                      rl.Vector3(-WIDTH/2,0,0),rl.Vector3(WIDTH/2,0 , 0)]
    plane_height = -HEIGHT/2
    
    mouse_state = MouseState.CAPTURED
    if mouse_state==MouseState.CAPTURED: rl.disable_cursor()

    DUDE_SPEED = 80
    can_fly_ptr = rl.ffi.new("bool *", False)
    can_fly = can_fly_ptr[0]
    y_velocity = 0
    texture, model, ray_image = None, None, None
    
    
        
    # MAIN LOOP
    last_capture = 0
    while(not rl.window_should_close()):
        # ---MOVE---
        dt = rl.get_frame_time()
        if (rl.is_key_pressed(K.KEY_TAB)):
            if mouse_state == MouseState.CAPTURED:
                rl.enable_cursor()
                mouse_state = MouseState.ACTIVE
            else:
                rl.disable_cursor()
                mouse_state = MouseState.CAPTURED
        if (rl.is_key_down(K.KEY_LEFT_SHIFT)):
            speed = DUDE_SPEED*2
        else:
            speed = DUDE_SPEED
        
        if mouse_state == MouseState.CAPTURED:
            rot = rl.Vector3(rl.get_mouse_delta().x*0.1, rl.get_mouse_delta().y*0.1, 0)
        else:
            rot = rl.vector3_zero()
        input_delta = rl.Vector2((rl.is_key_down(K.KEY_W) - rl.is_key_down(K.KEY_S))*speed*dt,
                        (rl.is_key_down(K.KEY_D) - rl.is_key_down(K.KEY_A))*speed*dt)
        
        forward_dir = rl.vector3_normalize(rl.vector3_subtract(camera.target, camera.position))
        right_dir = rl.vector3_rotate_by_axis_angle(forward_dir, camera.up, -math.pi/2)
        dr = rl.vector3_add(rl.vector3_scale(forward_dir, input_delta.x), rl.vector3_scale(right_dir, input_delta.y))
                
        next_pos = rl.vector3_add(camera.position, dr)
        
        if not wall_positions[2].x <= next_pos.x <= wall_positions[3].x:
            dr = project(dr, rl.Vector3(0, 0, 1))
        elif not wall_positions[0].z <= next_pos.z <= wall_positions[1].z:
            dr = project(dr, rl.Vector3(1, 0, 0))

        camera.position = rl.vector3_add(camera.position, dr)
        camera.target = rl.vector3_add(camera.target, dr)
        rl.update_camera_pro(camera, rl.vector3_zero(), rot, 0)

        if can_fly:
            y_velocity = 0
        else:
            y_velocity += -GRAVITY*dt
        
        new_y = camera.position.y + y_velocity*dt
        
        jump = new_y - PLAYER_SIZE - 0.01 < plane_height
        if (new_y - PLAYER_SIZE < plane_height):
            new_y = plane_height + PLAYER_SIZE
            y_velocity = 0
        
        diff = new_y - camera.position.y 
        camera.position.y = new_y
        camera.target.y += diff
        
        if rl.is_key_down(K.KEY_SPACE):
            if jump and not can_fly:
                y_velocity = GRAVITY
            elif can_fly:
                camera.position.y += GRAVITY*dt
                camera.target.y += GRAVITY*dt
            
        if can_fly and rl.is_key_down(K.KEY_LEFT_CONTROL):
            camera.position.y -= GRAVITY/2*dt
            camera.target.y -= GRAVITY/2*dt
            
        max_height = max_height_ptr[0]
    
        # ---Capture---
        if FILE and last_capture == 0:
            mixer.music.play(-1)
    #if time.time() - last_capture > (1/fps):
        last_capture = time.time()
        # Free previous
        if texture and model and ray_image:
            rl.unload_texture(texture)
            rl.unload_model(model)
            rl.unload_image(ray_image)
        
        ret, frame = video.read()
        if not ret:
            print("WHAT")
            exit(1)
        frame = cv2.resize(frame, (WIDTH, HEIGHT))
        ray_image = get_image_from_frame(frame) 
        texture = rl.load_texture_from_image(ray_image)
        mario_plane.materials[0].maps[0].texture = texture
        rl.image_color_invert(ray_image)
        # Mesh
        mesh = rl.gen_mesh_heightmap(ray_image, rl.Vector3(WIDTH, max_height, HEIGHT))
        model = rl.load_model_from_mesh(mesh)
        model.materials[0].maps[rl.MaterialMapIndex.MATERIAL_MAP_ALBEDO].texture = texture

        # ---Drawing---
        rl.begin_drawing()
        rl.clear_background(rl.SKYBLUE)
        #rl.draw_texture(mario_text, 0, 0, rl.WHITE)
        
        rl.begin_mode_3d(camera)
        
        # DUUUUUUUUDE
        pos = wall_positions[0]
        transform = rl.matrix_translate(-WIDTH/2, 0, -HEIGHT/2)
        transform = rl.matrix_multiply(transform, rl.matrix_rotate_x(math.pi/2))

        model.transform = transform
        rl.draw_model(model, pos, 1, rl.WHITE)
        
        model.transform = rl.matrix_multiply(transform, rl.matrix_rotate_y(math.pi))
        rl.draw_model(model, wall_positions[1], 1, rl.WHITE)

        model.transform = rl.matrix_multiply(transform, rl.matrix_rotate_y(math.pi/2))
        rl.draw_model(model, wall_positions[2], 1, rl.WHITE)
        
        model.transform = rl.matrix_multiply(transform, rl.matrix_rotate_y(-math.pi/2))
        rl.draw_model(model, wall_positions[3], 1, rl.WHITE)

        # Plane
        rl.draw_model(mario_plane, rl.Vector3(0,plane_height, 0), 1, rl.WHITE)
        rl.draw_model_ex(mario_plane, rl.Vector3(0, -plane_height, 0), rl.Vector3(1,0,0), 180, rl.vector3_one(), rl.WHITE)
        
        rl.end_mode_3d()

        # ---UI---
        # rl.draw_texture_ex(rl.load_texture_from_image(ray_image), rl.Vector2(0, 0),0, 2, rl.WHITE)
        rl.draw_fps(10, 10)
        text = f"Video: {fps}fps" 
        rl.draw_text(text, 10, 40, 20, rl.DARKBLUE)
        rl.draw_text(f"goofy size: {int(max_height)}", 10, 70, 20, rl.DARKBLUE)
        rl.gui_slider(rl.Rectangle(10, 90, 150, 20), "", "", max_height_ptr, 0.0, 480.0)
        rl.draw_text("Fly?", 10, 120, 20, rl.DARKBLUE)
        
        rl.gui_toggle(rl.Rectangle(10, 140, 20, 20), "", can_fly_ptr)
        can_fly = can_fly_ptr[0]
        
        text_direction = f"Forward: ({round(forward_dir.x,1)},{round(forward_dir.z,1)})"
        rl.draw_text(text_direction, 1050, 20, 20, rl.RED)
        
        start = rl.Vector2(1140, 120)
        v2_forward = rl.vector2_normalize(rl.Vector2(forward_dir.x, forward_dir.z))
        v2_right = rl.vector2_normalize(rl.Vector2(right_dir.x, right_dir.z))
        up = rl.vector2_add(start, rl.vector2_scale(v2_forward, 60))
        right = rl.vector2_add(start, rl.vector2_scale(v2_right, 60))
        
        # FUCK TRIANGLES
        rl.draw_line_ex(start, up, 3, rl.RED)
        v3 = rl.vector2_add(start, rl.vector2_rotate(rl.vector2_scale(v2_forward, 50), math.pi/20))
        v2 = rl.vector2_add(start, rl.vector2_rotate(rl.vector2_scale(v2_forward, 50), -math.pi/20))
        rl.draw_triangle(up, v2, v3, rl.RED)
        rl.draw_line_ex(start, right, 3, rl.BLUE)
        v3 = rl.vector2_add(start, rl.vector2_rotate(rl.vector2_scale(v2_right, 50), math.pi/20))
        v2 = rl.vector2_add(start, rl.vector2_rotate(rl.vector2_scale(v2_right, 50), -math.pi/20))
        rl.draw_triangle(right, v2, v3, rl.BLUE)
        
        rl.end_drawing()
        # ---------------
        
    rl.close_window()

if __name__ == "__main__":
    main()