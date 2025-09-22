### Video to TUM
To convert a video to TUM format please use the conversion.sh file. Simply do:
```
bash conversion.sh
```
and follow the steps presented in ther terminal. However, make sure this file is placed in the same directory as the video. 

### COLMAP to TUM format
To convert a COLMAP model to a MONO TUM ORB SLAM output first save the COLMAP as a text model. After you've done this locate the directory its been saved to and run this command. 
```
cat images.txt | grep .png > poses.txt
```
This will create a poses.txt file which can be used by the convertCOLtoMONOTUM.py script. To run the script run 

```
python3 convertCOLtoMONOTUM.py /path/to/poses.txt
```
This will now create a output.txt file which can be used by the EVO tool. 