<?php

include('function.php');

header('Access-Control-Allow-Origin:*');
header('Content-Type: application/json');
header('Access-Content-Allow-Method: GET');
header('Access-Control-Allow-Headers: Content-Type, Access-Control-Allow-Headers, Authorization, X-Request-With');

$requestMethod = $_SERVER["REQUEST_METHOD"];

if($requestMethod == "GET"){
    $scoreList = getAllScores();
    echo $scoreList;
} else {
    $data = [
        'status' => 405,
        'message' => $requestMethod . ' Method Not Allowed'
    ];
    header("HTTP/1.0. 405 Method not Allowed");
    echo json_encode($data);
}

?>