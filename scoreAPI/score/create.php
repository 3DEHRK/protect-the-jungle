<?php

include('function.php');

header('Access-Control-Allow-Origin:*');
header('Content-Type: application/json');
header('Access-Content-Allow-Method: POST');
header('Access-Control-Allow-Headers: Content-Type, Access-Control-Allow-Headers, Authorization, X-Request-With');

$requestMethod = $_SERVER["REQUEST_METHOD"];

if($requestMethod == "POST"){

    $inputData = json_decode(file_get_contents("php://input"), true);
    $storeScore = storeScore($inputData);

    echo $storeScore;
} else {
    $data = [
        'status' => 405,
        'message' => $requestMethod . ' Method Not Allowed'
    ];
    header("HTTP/1.0. 405 Method not Allowed");
    echo json_encode($data);
}


?>