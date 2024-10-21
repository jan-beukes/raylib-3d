import pyray as rl
from pyray import KeyboardKey as K
import cv2
import math
import random
import time

WIDTH = 640
HEIGHT = 480
CAPTURE_FPS = 25.0
GRAVITY = 200
PLAYER_SIZE = 10

def get_image_from_frame(frame: cv2.Mat) -> rl.Image:
    height = frame.shape[0]
    width = frame.shape[1]
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    image = rl.image_copy(rl.Image(frame.tobytes(), width, height, 1, rl.PixelFormat.PIXELFORMAT_UNCOMPRESSED_R8G8B8))
    return image
    
def main():
    rl.init_window(1280, 720, "THING")
    video = cv2.VideoCapture("res/rick2.webm")
    rl.set_target_fps(120)
    
    mario = rl.load_image("res/mario.png")
    mario_text = rl.load_texture_from_image(mario)
    mario_plane: rl.Model = rl.load_model_from_mesh(rl.gen_mesh_plane(WIDTH, WIDTH, 10, 10))
    mario_plane.materials[0].maps[0].texture = mario_text


    max_height = 80
    camera: rl.Camera3D = rl.Camera3D(rl.Vector3(0,0,0), rl.Vector3(0, 0,-1), 
                rl.Vector3(0, 1, 0), 120.0, rl.CameraProjection.CAMERA_PERSPECTIVE)
    rl.disable_cursor()
    
    wall_positions = [rl.Vector3(0,0,-WIDTH/2),rl.Vector3(0,0, WIDTH/2),
                      rl.Vector3(-WIDTH/2,0,0),rl.Vector3(WIDTH/2,0 , 0)]
    plane_height = -HEIGHT/2
    
    DUDE_SPEED = 60
    y_velocity = 0
    # MAIN LOOP
    last_capture = 0
    texture, model, ray_image = None, None, None
    while(not rl.window_should_close()):
        # Move
        dt = rl.get_frame_time()
        if (rl.is_key_down(K.KEY_LEFT_SHIFT)):
            speed = DUDE_SPEED*2
        else:
            speed = DUDE_SPEED
        
        rot = rl.Vector3(rl.get_mouse_delta().x*0.1, rl.get_mouse_delta().y*0.1, 0)
        
        dr = rl.Vector3((rl.is_key_down(K.KEY_W) - rl.is_key_down(K.KEY_S)) * speed * dt,
                      (rl.is_key_down(K.KEY_D) - rl.is_key_down(K.KEY_A)) * speed * dt, 0)
        
        player = rl.Vector2(camera.position.x, camera.position.z)
        target = rl.Vector2(camera.target.x, camera.target.z)
        forward_dir = rl.vector2_normalize(rl.vector2_subtract(target, player))
        right_dir = rl.vector2_rotate(forward_dir, math.pi/2)
        displace = rl.vector2_add(rl.vector2_scale(forward_dir, dr.x), rl.vector2_scale(right_dir, dr.y))
        
        next_pos = rl.vector2_add(rl.Vector2(camera.position.x, camera.position.z), displace)
        
        if not wall_positions[2].x <= next_pos.x <= wall_positions[3].x:
            dr = rl.vector3_zero()
        if not wall_positions[0].z <= next_pos.y <= wall_positions[1].z:
            dr = rl.vector3_zero()
                 
        rl.update_camera_pro(camera, dr, rot, 0)
        
        jump = False
        y_velocity += -GRAVITY*dt
        new_y = camera.position.y + y_velocity*dt
        
        if (new_y - PLAYER_SIZE < plane_height):
            new_y = plane_height + PLAYER_SIZE
            y_velocity = 0
            jump = True
        
        diff = new_y - camera.position.y 
        camera.position.y = new_y
        camera.target.y += diff
        
        if (rl.is_key_down(K.KEY_SPACE) and jump):
            y_velocity = GRAVITY
        
       
        if rl.get_time() - last_capture > (1/CAPTURE_FPS):
            # Capture
            last_capture = rl.get_time()
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

        # rl.draw_texture_ex(rl.load_texture_from_image(ray_image), rl.Vector2(0, 0),0, 2, rl.WHITE)
        rl.draw_fps(10, 10)
        text = f"Image: {ray_image.width} {ray_image.height}"
        rl.draw_text(text, 10, 30, 20, rl.RAYWHITE)
        start = rl.Vector2(1200, 40)
        up = rl.vector2_add(start, rl.vector2_scale(forward_dir, 30))
        right = rl.vector2_add(start, rl.vector2_scale(right_dir, 30))
        
        rl.draw_line(int(start.x), int(start.y), int(up.x), int(up.y), rl.BLUE)
        rl.draw_line(int(start.x), int(start.y), int(right.x), int(right.y), rl.RED)
        
        rl.end_drawing()
        # ---------------
        
    rl.close_window()

if __name__ == "__main__":
    main()