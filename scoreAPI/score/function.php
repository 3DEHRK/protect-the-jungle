<?php

require '../inc/dbcon.php';

function getAllScores(){

    global $conn;
    $query = "SELECT name, score, RANK() OVER (ORDER BY score DESC) AS 'rank' FROM score";
    $query_run = mysqli_query($conn,$query);

    if($query_run){

        if(mysqli_num_rows($query_run) >0) {

            $res = mysqli_fetch_all($query_run, MYSQLI_ASSOC);

            $data = [
                        'status' => 200,
                        'message' => 'Scores Fetched Successfully',
                        'data' => $res
                    ];

            header("HTTP/1.0. 200 OK");
            return json_encode($data);
        } else {
            $data = [
                        'status' => 404,
                        'message' => 'No Score Found'
                    ];
            header("HTTP/1.0. 404 No Score Found");
            return json_encode($data);
        }


    } else {
        $data = [
            'status' => 500,
            'message' => 'Internal Server Error'
        ];
        header("HTTP/1.0. 500 Internal Server Error");
        return json_encode($data);
    }

}

function storeScore($score){

    global $conn;

    $name = mysqli_real_escape_string($conn, $score['name']);
    $score = mysqli_real_escape_string($conn, $score['score']);

    if (empty(trim($name))) {
        return error422('Enter you name');
    } elseif (empty(trim($score))) {
        return error422('Enter you score');
    } else {
        $insert = "INSERT INTO score (name, score) VALUES ('$name', $score)";
        $res = mysqli_query($conn, $insert);
        $id = mysqli_insert_id($conn);
        $query = "SELECT id, RANK() OVER (ORDER BY score DESC) AS 'rank' FROM score";
        $query_run = mysqli_query($conn, $query);
        $ranks = mysqli_fetch_all($query_run);

        foreach ($ranks as $rank){
            if ($rank[0] == $id) {
                $resultRank = $rank[1];
            }
        }

        if ($res) {
            $data = [
                    'status' => 201,
                    'message' => 'Scores Created Successfully',
                    'data' => [
                        'rank' => $resultRank
                    ]
                ];
            header("HTTP/1.0. 201 Created");
            return json_encode($data);
        } else {
            $data = [
                'status' => 500,
                'message' => 'Internal Server Error'
            ];
            header("HTTP/1.0. 500 Internal Server Error");
            return json_encode($data);
        }
    }


}

function error422($message) {
    $data = [
                'status' => 422,
                'message' => $message
            ];
            header("HTTP/1.0. 500 Unprocessable Entity");
            echo json_encode($data);
            exit();
}

?>