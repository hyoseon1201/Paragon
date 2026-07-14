package com.p1.backend.controller;

import com.p1.backend.dto.ErrorResponse;
import com.p1.backend.dto.LoginRequest;
import com.p1.backend.dto.LoginResponse;
import com.p1.backend.dto.SignupRequest;
import com.p1.backend.dto.SignupResponse;
import com.p1.backend.service.AuthService;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/auth")
public class AuthController {

    private final AuthService authService;

    public AuthController(AuthService authService) {
        this.authService = authService;
    }

    @PostMapping("/signup")
    public ResponseEntity<?> signup(@RequestBody SignupRequest request) {
        try {
            authService.signup(request.username(), request.password());
            return ResponseEntity.status(HttpStatus.CREATED).body(new SignupResponse(request.username()));
        } catch (AuthService.UsernameTakenException e) {
            return ResponseEntity.status(HttpStatus.CONFLICT).body(new ErrorResponse("USERNAME_TAKEN"));
        }
    }

    @PostMapping("/login")
    public ResponseEntity<?> login(@RequestBody LoginRequest request) {
        try {
            String token = authService.login(request.username(), request.password());
            return ResponseEntity.ok(new LoginResponse(token));
        } catch (AuthService.InvalidCredentialsException e) {
            return ResponseEntity.status(HttpStatus.UNAUTHORIZED).body(new ErrorResponse("INVALID_CREDENTIALS"));
        }
    }
}
